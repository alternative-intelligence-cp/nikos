/*
 * Path traversal test: getenv() -> snprintf -> fopen
 *
 * Expected: UNSAFE (PathTraversal at fopen call)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char path[256];
    const char* filename = getenv("DOCUMENT");
    if (!filename) return 1;

    /* Tainted filename flows directly into file open */
    snprintf(path, sizeof(path), "/var/www/docs/%s", filename);
    FILE* f = fopen(path, "r"); /* UNSAFE: PathTraversal */
    if (f) fclose(f);
    return 0;
}
