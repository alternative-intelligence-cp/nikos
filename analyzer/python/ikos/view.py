###############################################################################
#
# IKOS web view
#
# Author: Thomas Bailleux
#
# Contributor: Maxime Arthaud
#
# Contact: ikos@lists.nasa.gov
#
# Notices:
#
# Copyright (c) 2018-2023 United States Government as represented by the
# Administrator of the National Aeronautics and Space Administration.
# All Rights Reserved.
#
# Disclaimers:
#
# No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF
# ANY KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED
# TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS,
# ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
# OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE
# ERROR FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED, WILL CONFORM TO
# THE SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN
# ENDORSEMENT BY GOVERNMENT AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS,
# RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS
# RESULTING FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY
# DISCLAIMS ALL WARRANTIES AND LIABILITIES REGARDING THIRD-PARTY SOFTWARE,
# IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."
#
# Waiver and Indemnity:  RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST
# THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL
# AS ANY PRIOR RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE RESULTS
# IN ANY LIABILITIES, DEMANDS, DAMAGES, EXPENSES OR LOSSES ARISING FROM SUCH
# USE, INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM,
# RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD
# HARMLESS THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS,
# AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW.
# RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE IMMEDIATE,
# UNILATERAL TERMINATION OF THIS AGREEMENT.
#
###############################################################################
import argparse
import collections
import html
import io
import json
import operator
import os
import os.path
import re
import sqlite3
import sys
import threading
import webbrowser

from ikos import args
from ikos import colors
from ikos import log
from ikos import report
from ikos import settings
from ikos.enums import Result, CheckKind, CheckerName
from ikos.highlight import CppLexer, HtmlFormatter, highlight
from ikos.http import HTTPServerIPv6, BaseHTTPRequestHandler
from ikos.log import printf
from ikos.output_db import OutputDatabase


# Path to web resources
SHARE_DIR = os.path.join(settings.PREFIX, 'share', 'ikos', 'view')


class TemplateEngine:
    ''' Simple template engine for formatting HTML page '''

    _singleton = None

    @classmethod
    def get(cls):
        ''' Get the singleton instance of the template engine '''
        if cls._singleton is None:
            cls._singleton = TemplateEngine()

        return cls._singleton

    def __init__(self):
        self._cache = {}

    def _read(self, fullpath):
        ''' Read a template from a file or cache '''
        if fullpath in self._cache and settings.BUILD_MODE == 'Release':
            log.debug("Using '%s' from cache" % fullpath)
            return self._cache[fullpath]
        else:
            with io.open(fullpath, 'r',
                         encoding='utf-8',
                         errors='ignore') as f:
                data = f.read()
            self._cache[fullpath] = data
            return data

    def process(self, path, values={}):
        '''
        Format an HTML page with a given template and a dictionary of values
        '''
        # read the template file
        fullpath = os.path.join(SHARE_DIR, 'template', path)
        data = self._read(fullpath)

        # decode bytes as utf-8
        for key in values:
            if isinstance(values[key], bytes):
                values[key] = values[key].decode('utf-8')

        # substitute {xxx} placeholders
        try:
            data = data.format(**values)
        except KeyError:
            pass

        return data


class RequestHandler(BaseHTTPRequestHandler):
    ''' Handler for an HTTP request '''

    _static_cache = {}

    def do_GET(self):
        ''' Handle a GET request '''
        urls = [
            (r'^/$',
             self._serve_homepage),
            (r'^/static/(?P<path>[a-zA-Z_-]+/[a-zA-Z0-9_-]+\.[a-zA-Z_-]+)$',
             self._serve_static),
            (r'^/settings$',
             self._serve_settings),
            (r'^/report/(?P<id>[0-9]+)(\?k=(?P<kinds_filter>[A-Z0-9]+))?$',
             self._serve_report),
            (r'^/api/v1/summary$',
             self._serve_api_summary),
            (r'^/api/v1/sarif$',
             self._serve_api_sarif),
            (r'^/api/v1/checks$',
             self._serve_api_checks),
            (r'^/api/v1/checks/(?P<check_id>[0-9]+)$',
             self._serve_api_check),
            (r'^/api/v1/files$',
             self._serve_api_files),
            (r'^/api/v1/functions$',
             self._serve_api_functions),
        ]

        # Strip query string before routing; individual handlers parse it if needed
        path = self.path.split('?', 1)[0]

        for pattern, f in urls:
            m = re.match(pattern, path)
            if m:
                f(**m.groupdict())
                return

        self._serve_not_found()

    # API helpers

    def _send_json(self, data, status=200):
        ''' Write a JSON response with CORS headers '''
        body = json.dumps(data, indent=2).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json; charset=UTF-8')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    # API Endpoints

    def _serve_api_summary(self):
        ''' GET /api/v1/summary — analysis summary counts '''
        view_report = View.get().report
        summary = report.generate_summary(view_report.db)
        self._send_json({
            'ok': summary.ok,
            'error': summary.error,
            'warning': summary.warning,
            'unreachable': summary.unreachable,
            'total': summary.total
        })

    def _serve_api_sarif(self):
        ''' GET /api/v1/sarif — full SARIF 2.1.0 report '''
        view_report = View.get().report

        output = io.StringIO()
        fmt = report.SARIFFormatter(output, 4)
        fmt.format(view_report._report)

        body = output.getvalue().encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', 'application/sarif+json; charset=UTF-8')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _serve_api_checks(self):
        '''
        GET /api/v1/checks — all checks as a JSON array.

        Supported query parameters (all optional, combinable):
          ?status=error|warning|safe|unreachable
          ?kind=<short-name>   (e.g. boa, dbz, taint)
          ?file_id=<int>
        '''
        from urllib.parse import urlparse, parse_qs
        db = View.get().db

        parsed = urlparse(self.path)
        params = parse_qs(parsed.query)

        # Build WHERE clause
        where_clauses = []
        sql_params = []

        if 'status' in params:
            status_map = {
                'error': Result.ERROR,
                'warning': Result.WARNING,
                'safe': Result.OK,
                'unreachable': Result.UNREACHABLE,
            }
            status_val = params['status'][0].lower()
            if status_val in status_map:
                where_clauses.append('c.status = ?')
                sql_params.append(status_map[status_val])

        if 'kind' in params:
            try:
                kind_int = CheckKind.from_short_name(params['kind'][0])
                where_clauses.append('c.kind = ?')
                sql_params.append(int(kind_int))
            except Exception:
                pass

        if 'file_id' in params:
            where_clauses.append('s.file_id = ?')
            sql_params.append(int(params['file_id'][0]))

        where = ('WHERE ' + ' AND '.join(where_clauses)) if where_clauses else ''

        sql = '''
            SELECT c.id, c.kind, c.checker, c.status,
                   c.statement_id, c.call_context_id,
                   s.line, s.column, s.file_id,
                   f.id as func_id, f.demangled
            FROM checks c
            JOIN statements s ON c.statement_id = s.id
            JOIN functions f ON s.function_id = f.id
            %s
            ORDER BY s.file_id, s.line, s.column
        ''' % where

        cursor = db.con.cursor()
        cursor.execute(sql, sql_params)

        checks = []
        for row in cursor.fetchall():
            (cid, kind, checker, status, stmt_id, ctx_id,
             line, col, file_id, func_id, demangled) = row
            checks.append({
                'id': cid,
                'kind': CheckKind.short_name(kind),
                'kind_long': CheckKind.long_name(kind),
                'checker': CheckerName.short_name(checker),
                'status': Result.str(status),
                'statement_id': stmt_id,
                'call_context_id': ctx_id,
                'line': line,
                'column': col,
                'file_id': file_id,
                'function': demangled,
            })
        cursor.close()
        self._send_json(checks)

    def _serve_api_check(self, check_id):
        '''
        GET /api/v1/checks/{id} — single check detail with operands and
        call-context chain.
        '''
        from ikos.output_db import Check as OCheck
        db = View.get().db
        cid = int(check_id)

        cursor = db.con.cursor()
        cursor.execute('SELECT * FROM checks WHERE id = ?', (cid,))
        row = cursor.fetchone()
        cursor.close()

        if row is None:
            self._send_json({'error': 'check %d not found' % cid}, status=404)
            return

        chk = OCheck(row, db)
        stmt = chk.statement()
        ctx = chk.call_context()

        # Build call chain
        chain = []
        c = ctx
        while not c.empty():
            cs = c.call()
            fn = c.function()
            chain.append({
                'file': report.format_path(cs.file_path()),
                'line': cs.line_or(None),
                'column': cs.column_or(None),
                'function': fn.pretty_name(),
            })
            c = c.parent()

        # Operands
        operands = []
        raw_ops = chk.load_operands()
        if raw_ops:
            for pair in raw_ops:
                operands.append({'num': pair.num, 'repr': pair.operand.repr})

        self._send_json({
            'id': chk.id,
            'kind': CheckKind.short_name(chk.kind),
            'kind_long': CheckKind.long_name(chk.kind),
            'checker': CheckerName.short_name(chk.checker),
            'status': Result.str(chk.status),
            'file': report.format_path(stmt.file_path()),
            'line': stmt.line_or(None),
            'column': stmt.column_or(None),
            'function': stmt.function().pretty_name(),
            'operands': operands,
            'call_chain': chain,
            'info': chk.info,
        })

    def _serve_api_files(self):
        ''' GET /api/v1/files — all source files with per-status check counts '''
        view_report = View.get().report
        files = []
        for f in view_report.files:
            sk = view_report.files_status_kinds.get(f.id, {})
            # sk is a StatusKinds namedtuple indexed by Result int
            def _count(result_int):
                try:
                    d = sk[result_int]
                    return sum(d.values()) if isinstance(d, dict) else 0
                except (KeyError, TypeError):
                    return 0
            files.append({
                'id': f.id,
                'path': report.format_path(f.path),
                'errors': _count(Result.ERROR),
                'warnings': _count(Result.WARNING),
                'safe': _count(Result.OK),
                'unreachable': _count(Result.UNREACHABLE),
            })
        files.sort(key=lambda x: x['path'])
        self._send_json(files)

    def _serve_api_functions(self):
        ''' GET /api/v1/functions — all functions with name, file, and line '''
        db = View.get().db
        functions = []
        for f in db.functions:
            functions.append({
                'id': f.id,
                'name': f.name,
                'demangled': f.demangled,
                'file': report.format_path(f.file().path) if f.file() else None,
                'line': f.line,
                'is_definition': f.definition,
            })
        functions.sort(key=lambda x: (x['file'] or '', x['line'] or 0))
        self._send_json(functions)

    # Static files

    def _serve_static(self, path):
        ''' Serve a static file '''
        fullpath = os.path.join(SHARE_DIR, 'static', path)

        if os.path.isfile(fullpath):
            self._send_static_headers(path)
            self._write_file(fullpath)
        else:
            self._serve_not_found()

    # Homepage

    def _serve_homepage(self):
        ''' Serve the homepage '''
        self._write_template('homepage.html', {
            'check_kinds': json.dumps(self._check_kinds()),
            'check_kinds_filter': json.dumps(self._check_kinds_filter()),
            'files': json.dumps(self._files()),
        })

    def _check_kinds(self):
        ''' Generate the Javascript variable check_kinds '''
        view_report = View.get().report
        return [{'id': kind, 'name': CheckKind.long_name(kind)}
                for kind in view_report.kinds]

    def _check_kinds_filter(self, param=None):
        ''' Generate the Javascript variable check_kinds_filter '''
        view_report = View.get().report

        filter = {kind: True for kind in view_report.kinds}

        if not param:
            return filter

        param = [int(param[i:i + 2], 16) for i in range(0, len(param), 2)]

        for kind in view_report.kinds:
            try:
                filter[kind] = (param[kind // 8] & (1 << (kind % 8))) != 0
            except IndexError:
                pass

        return filter

    def _files(self):
        ''' Generate the Javascript variable files '''
        view_report = View.get().report
        files = []

        for file in view_report.files:
            files.append({
                'id': file.id,
                'path': report.format_path(file.path),
                'status_kinds': view_report.files_status_kinds[file.id]
            })

        # Sort by path
        files.sort(key=operator.itemgetter('path'))

        return files

    # Settings

    def _serve_settings(self):
        ''' Serve the settings page '''
        db = View.get().db
        settings = db.load_settings()
        s = []

        for name, value in settings.items():
            if isinstance(value, list):
                s.append('<tr><td>%s</td><td><i>%s</i></td></tr>'
                         % (html.escape(name),
                            html.escape(json.dumps(value))))
            else:
                s.append('<tr><td>%s</td><td><i>%s</i></td></tr>'
                         % (html.escape(name),
                            html.escape(str(value))))

        self._write_template('settings.html', {'settings': '\n\t'.join(s)})

    # File report

    def _serve_report(self, id, kinds_filter):
        ''' Serve a specific report for a file '''
        id = int(id)
        view_report = View.get().report

        try:
            file = view_report.files[id]
        except IndexError:
            self._serve_not_found()
            return

        try:
            with io.open(file.path, 'r',
                         encoding='utf-8',
                         errors='ignore') as f:
                code = f.read()
        except (OSError, IOError):
            self._serve_error("No such file: %s" % file.path)
            return

        fmt = Formatter(file)
        lexer = CppLexer(stripnl=False)
        code = highlight(code, lexer, fmt)
        check_kinds_filter = self._check_kinds_filter(param=kinds_filter)
        self._write_template('report.html', {
            'filepath': html.escape(report.format_path(file.path)),
            'check_kinds': json.dumps(self._check_kinds()),
            'check_kinds_filter': json.dumps(check_kinds_filter),
            'code': code,
            'functions': json.dumps(fmt.functions),
            'call_contexts': json.dumps(fmt.call_contexts),
            'checks': json.dumps(fmt.checks),
            'pygments_css': fmt.get_style_defs('.highlight')
        })

    # Helpers

    def _send_static_headers(self, path):
        ''' Send header for a static file '''
        self.send_response(200)
        mimetype = 'text/plain'
        if path.endswith('.css'):
            mimetype = 'text/css'
        elif path.endswith('.js'):
            mimetype = 'application/javascript'
        elif path.endswith('.jpg'):
            mimetype = 'image/jpg'
        elif path.endswith('.png'):
            mimetype = 'image/png'
        self.send_header('Content-Type', '%s; charset=UTF-8' % mimetype)
        self.end_headers()

    def _write_file(self, fullpath):
        ''' Write a static file to the response stream '''
        if fullpath in RequestHandler._static_cache \
                and settings.BUILD_MODE == 'Release':
            self.wfile.write(RequestHandler._static_cache[fullpath])
            log.debug("Using '%s' from cache" % fullpath)
        else:
            with open(fullpath, 'rb') as f:
                data = f.read()
            self.wfile.write(data)
            RequestHandler._static_cache[fullpath] = data

    def _write_template(self, path, values={}, status=200):
        ''' Write a template to the response stream '''
        engine = TemplateEngine.get()
        self.send_response(status)
        self.send_header('Content-Type', 'text/html; charset=UTF-8')
        self.end_headers()
        self.wfile.write(engine.process(path, values).encode('utf8'))

    def _serve_not_found(self):
        ''' Server a 404 Not Found page '''
        self._write_template('not_found.html',
                             {'path': html.escape(self.path)},
                             status=404)

    def _serve_error(self, message):
        ''' Server a 500 Internal Error page '''
        self._write_template('error.html',
                             {'message': html.escape(message)},
                             status=500)


class View:
    ''' Class for IKOS view (ikos-view) '''

    _singleton = None

    @classmethod
    def get(cls):
        ''' Get the singleton instance of view '''
        assert cls._singleton is not None
        return cls._singleton

    def __init__(self, db, port=8080):
        View._singleton = self
        self.db = db
        self.port = port
        self.report = ViewReport(self.db)

        try:
            self.httpd = HTTPServerIPv6(('', self.port), RequestHandler)
        except (OSError, IOError) as e:
            log.error("Could not start the HTTP server: %s" % e)
            sys.exit(1)

    def serve(self):
        log.info("Pre-processing...")
        self.report.pre_process()
        log.info("Starting ikos-view at http://localhost:%d" % self.port)
        log.warning("Use Ctrl-C to exit")
        self.httpd.serve_forever()


StatusKinds = collections.namedtuple('StatusKinds',
                                     ['ok', 'warning', 'error', 'unreachable'])


class ViewReport:
    ''' IKOS view report '''

    def __init__(self, db):
        self.db = db
        self._report = None
        self.kinds = None
        self.files = None

        # Map[file.id, Map[check.status, Map[check.kind, count]]]
        self.files_status_kinds = None

        # Map[file.id, Map[check.line, List[Check]]]
        self.files_lines_reports = None

    def pre_process(self):
        ''' Pre processing some values '''
        # List of CheckKind
        c = self.db.con.cursor()
        rows = c.execute("SELECT DISTINCT kind FROM checks ORDER BY kind")
        self.kinds = [row[0] for row in rows]

        # Generate report
        self._report = report.generate_report(self.db)

        self.files = self.db.files
        self.files_status_kinds = {}
        self.files_lines_reports = {}
        for file in self.files:
            self.files_status_kinds[file.id] = StatusKinds(ok={},
                                                           warning={},
                                                           error={},
                                                           unreachable={})
            self.files_lines_reports[file.id] = {}

        for statement_report in self._report.statement_reports:
            stmt = statement_report.statement()
            file = stmt.file()

            if file is None:
                continue  # Ignore checks without a file

            kind = statement_report.kind
            status = statement_report.status

            # Update self.files_status_kinds
            kinds = self.files_status_kinds[file.id][status]
            if kind not in kinds:
                kinds[kind] = 0
            kinds[kind] += 1

            # Update self.files_lines_reports
            lines_reports = self.files_lines_reports[file.id]
            if stmt.line not in lines_reports:
                lines_reports[stmt.line] = []
            lines_reports[stmt.line].append(statement_report)


class Formatter(HtmlFormatter):
    ''' Source code HTML formatter '''

    def __init__(self, file):
        super(Formatter, self).__init__()
        self.file = file
        self.functions = {}
        self.call_contexts = {}
        self.checks = {}

    def wrap(self, source):
        return self._wrap_code(source)

    def _wrap_code(self, source):
        view_report = View.get().report
        lines_reports = view_report.files_lines_reports[self.file.id]

        yield 0, '<div class="highlight">\n'
        line_num = 1

        for i, line in source:
            statement_reports = lines_reports.get(line_num)

            t = '''<div id="L%d" class="line_wrap status_%d">
                     <span class="line_number">
                      <a href="#L%d">%d</a>
                     </span>
                     <span class="line_content">
                       <span class="line_code_wrap">
                         <span class="line_code">
                           <pre>%s</pre>
                         </span>
                         <span class="checks_toggle hidden">
                           <a href="javascript:" class="toggle">&#128065;</a>
                         </span>
                       </span>
                       <span class="checks hidden"></span>
                     </span>
                   </div>\n''' % (line_num,
                                  self._line_status(statement_reports),
                                  line_num,
                                  line_num,
                                  line.replace('\n', ''))

            if statement_reports:
                self.checks[line_num] = self._build_checks(statement_reports)

            yield i, t
            line_num += 1

        yield 0, '</div>'

    def _line_status(self, statement_reports):
        ''' Return the status of a source line '''
        if not statement_reports:
            return -1

        status = [0, 0, 0, 0]
        for statement_report in statement_reports:
            status[statement_report.status] += 1

        if status[Result.ERROR] > 0:
            return Result.ERROR
        elif status[Result.WARNING] > 0:
            return Result.WARNING
        elif status[Result.UNREACHABLE] > 0:
            return Result.UNREACHABLE
        else:
            return Result.OK

    def _build_checks(self, statement_reports):
        ''' Return the list of check for a source line '''
        checks = []

        for statement_report in statement_reports:
            statement = statement_report.statement()
            checks.append({
                'kind': statement_report.kind,
                'status': statement_report.status,
                'column': statement.column_or('?'),
                'message': report.generate_message(statement_report, 4),
                'function_id': statement.function_id,
                'call_context_ids': statement_report.call_context_ids,
            })
            self._build_function(statement.function())
            self._build_call_contexts(statement_report.call_contexts())

        # Sort by status (error > warning > ok)
        checks.sort(key=operator.itemgetter('status'), reverse=True)

        return checks

    def _build_function(self, function):
        if function.id in self.functions:
            return

        self.functions[function.id] = function.pretty_name()

    def _build_call_contexts(self, call_contexts):
        for call_context in call_contexts:
            self._build_call_context(call_context)

    def _build_call_context(self, call_context):
        call_context_id = call_context.id

        if call_context_id in self.call_contexts:
            return

        lines = []
        while not call_context.empty():
            call_statement = call_context.call()
            function = call_context.function()
            line = "%s:%s:%s: function '%s'" % (
                report.format_path(call_statement.file_path()) or '?',
                call_statement.line_or('?'),
                call_statement.column_or('?'),
                function.pretty_name()
            )
            lines.append(line)
            call_context = call_context.parent()

        self.call_contexts[call_context_id] = '\n'.join(lines)


##########################
# command line interface #
##########################


def parse_arguments(argv):
    usage = '%(prog)s [options] file.db'
    description = 'Launch ikos view on an output database'
    formatter_class = argparse.RawTextHelpFormatter
    parser = argparse.ArgumentParser(usage=usage,
                                     description=description,
                                     formatter_class=formatter_class)

    # Positional arguments
    parser.add_argument('file',
                        metavar='file.db',
                        help='Result database')

    # Optional arguments
    parser.add_argument('--version',
                        action=args.VersionAction,
                        nargs=0,
                        help='Show ikos version')
    parser.add_argument('--color',
                        dest='color',
                        metavar='',
                        help=args.help('Enable terminal colors:',
                                       args.color_choices,
                                       args.default_color),
                        choices=args.choices(args.color_choices),
                        default=args.default_color)
    parser.add_argument('--log',
                        dest='log_level',
                        metavar='',
                        help=args.help('Log level:',
                                       args.log_levels,
                                       args.default_log_level),
                        choices=args.choices(args.log_levels),
                        default='info')
    parser.add_argument('--port',
                        dest='port',
                        metavar='',
                        help='Listening port',
                        default=8080,
                        type=int)
    parser.add_argument('--api-only',
                        dest='api_only',
                        action='store_true',
                        help='Run headlessly without serving HTML or opening a browser',
                        default=False)

    return parser.parse_args(argv)


def open_browser(url):
    browser = webbrowser.get()

    if browser.name in ('links', 'elinks', 'lynx', 'w3m'):
        return

    log.debug('Launching browser')
    browser.open_new_tab(url)


######################
# main for ikos-view #
######################

def main(argv):
    progname = os.path.basename(argv[0])

    # parse arguments
    opt = parse_arguments(argv[1:])

    # setup colors and logging
    colors.setup(opt.color, file=log.out)
    log.setup(opt.log_level)

    if not os.path.isfile(opt.file):
        printf("%s: error: no such file: \'%s\'\n",
               progname, opt.file, file=sys.stderr)
        sys.exit(1)

    try:
        # open result database
        db = OutputDatabase(opt.file)

        v = View(db, port=opt.port)
        if not opt.api_only:
            browser_timer = threading.Timer(0.1,
                                            open_browser,
                                            ['http://localhost:%d/' % opt.port])
            browser_timer.start()
        v.serve()

        # close database
        db.close()
    except sqlite3.DatabaseError as e:
        printf('%s: error: %s\n', progname, e, file=sys.stderr)
        sys.exit(1)
