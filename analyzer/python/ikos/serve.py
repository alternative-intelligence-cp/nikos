import argparse
import sys
import uvicorn
from fastapi import FastAPI, HTTPException, Query
from typing import Optional, List
import json

from ikos import output_db
from ikos import triage
from ikos.enums import Result

app = FastAPI(title="NIKOS Unified Reporting API", description="REST wrapper for NIKOS SQLite database.")
db = None

@app.on_event("startup")
def startup_event():
    global db
    db_path = app.state.db_path
    try:
        db = output_db.OutputDatabase(db_path)
    except Exception as e:
        print(f"Error opening database {db_path}: {e}")
        sys.exit(1)

@app.on_event("shutdown")
def shutdown_event():
    if db:
        db.close()

@app.get("/api/info")
def get_info():
    """Return database metadata and analysis settings."""
    settings = db.load_settings()
    return {"settings": settings}

@app.get("/api/metrics")
def get_metrics():
    """Return analysis performance metrics."""
    times = db.load_timing_results()
    metrics = [{"pass": t[0], "time": t[1]} for t in times]
    return {"metrics": metrics}

@app.get("/api/files/{file_id}")
def get_file(file_id: int):
    """Return file path information."""
    for f in db.files:
        if f.id == file_id:
            return {"id": f.id, "path": f.path}
    raise HTTPException(status_code=404, detail="File not found")

@app.get("/api/checks")
def get_checks(status: Optional[str] = None, check_kind: Optional[str] = None):
    """Return all checks with optional filtering."""
    c = db.con.cursor()
    query = 'SELECT * FROM checks WHERE 1=1'
    params = []
    
    if status:
        try:
            status_enum = Result.from_str(status.lower())
            query += ' AND status = ?'
            params.append(status_enum)
        except KeyError:
            raise HTTPException(status_code=400, detail=f"Invalid status: {status}")
            
    if check_kind:
        query += ' AND kind = ?'
        params.append(check_kind)
        
    c.execute(query, params)
    checks = []
    for row in c:
        # row: id, kind, checker, status, statement_id, call_context_id, operands_id, info
        info_dict = {}
        if row[7]:
            try:
                info_dict = json.loads(row[7])
            except:
                pass
                
        checks.append({
            "id": row[0],
            "kind": row[1],
            "checker": row[2],
            "status": Result.str(row[3]),
            "statement_id": row[4],
            "call_context_id": row[5],
            "info": info_dict
        })
    return {"checks": checks}

def get_check_details(check_id: int):
    c = db.con.cursor()
    c.execute('SELECT kind, status, statement_id, info FROM checks WHERE id = ?', (check_id,))
    row = c.fetchone()
    if not row:
        raise HTTPException(status_code=404, detail="Check not found")
        
    kind, status, statement_id, info = row
    
    # get statement
    c.execute('SELECT file_id, line FROM statements WHERE id = ?', (statement_id,))
    stmt_row = c.fetchone()
    filepath = ""
    line_num = 0
    if stmt_row:
        file_id, line_num = stmt_row
        if file_id:
            c.execute('SELECT path FROM files WHERE id = ?', (file_id,))
            f_row = c.fetchone()
            if f_row:
                filepath = f_row[0]
                
    info_dict = {}
    if info:
        try:
            info_dict = json.loads(info)
        except:
            pass
            
    return kind, Result.str(status), filepath, line_num, info_dict

@app.get("/api/checks/{check_id}/explain")
def explain_check(check_id: int):
    """Invoke AI to explain the finding."""
    kind, status, filepath, line_num, info_dict = get_check_details(check_id)
    client = triage.get_client()
    explanation = client.explain_finding(kind, status, filepath, line_num, info_dict)
    return {"id": check_id, "explanation": explanation}

@app.get("/api/checks/{check_id}/fix")
def fix_check(check_id: int):
    """Invoke AI to suggest a unified diff fix for the finding."""
    kind, status, filepath, line_num, info_dict = get_check_details(check_id)
    client = triage.get_client()
    diff = client.suggest_fix(kind, filepath, line_num, info_dict)
    return {"id": check_id, "suggested_fix": diff}

def main():
    parser = argparse.ArgumentParser(description="NIKOS Unified Reporting API Server")
    parser.add_argument("db", help="Path to the NIKOS SQLite output database")
    parser.add_argument("--host", default="127.0.0.1", help="Host interface to bind to")
    parser.add_argument("--port", type=int, default=8000, help="Port to listen on")
    args = parser.parse_args()
    
    app.state.db_path = args.db
    uvicorn.run(app, host=args.host, port=args.port)

if __name__ == "__main__":
    main()
