// fgets_source.c
//
// Taint test: fgets() from stdin as a source.
// User input read from stdin via fgets() is untrusted (tainted).
// When used as a file path argument, this is a path traversal vulnerability.
//
// Expected: error (PathTraversal)
#include <stdio.h>

int main(int argc, char* argv[]) {
    char path[256];

    // fgets from stdin — user-controlled input
    if (fgets(path, sizeof(path), stdin) == NULL) {
        return 1;
    }

    // Strip trailing newline if present (value still tainted)
    int len = 0;
    while (path[len] && path[len] != '\n') len++;
    path[len] = '\0';

    // ERROR: user-supplied path used in fopen() without sanitization
    FILE* f = fopen(path, "r");
    if (f) {
        fclose(f);
    }

    return 0;
}
