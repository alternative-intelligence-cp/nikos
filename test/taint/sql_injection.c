/*
 * SQL injection test: getenv() -> snprintf -> sqlite3_exec
 *
 * Expected: UNSAFE (SQLInjection at sqlite3_exec call)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Opaque declarations so we don't need real SQLite headers */
typedef void sqlite3;
extern int sqlite3_exec(sqlite3* db, const char* sql, void* cb, void* arg, char** errmsg);

int main(void) {
    sqlite3* db = NULL;
    char query[256];
    const char* user_id = getenv("USER_ID");
    if (!user_id) return 1;

    /* Tainted user input flows directly into SQL query string */
    snprintf(query, sizeof(query), "SELECT * FROM users WHERE id='%s'", user_id);
    sqlite3_exec(db, query, NULL, NULL, NULL); /* UNSAFE: SQLInjection */
    return 0;
}
