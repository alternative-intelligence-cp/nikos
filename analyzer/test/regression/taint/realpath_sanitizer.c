// realpath_sanitizer.c
//
// Taint test: realpath() as a path sanitizer.
// User input from fgets() is tainted. Calling realpath() resolves the path
// to a canonical absolute path, which is treated as a sanitizer — taint
// is cleared on the output buffer.
//
// Expected: safe (PathTraversal cleared by realpath sanitizer)
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int main(int argc, char* argv[]) {
    char input[256];

    // fgets from stdin — tainted
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 1;
    }

    // Strip trailing newline
    int len = 0;
    while (input[len] && input[len] != '\n') len++;
    input[len] = '\0';

    // realpath() is a sanitizer: resolves symlinks, normalizes the path,
    // and ensures the result is an absolute canonical path.
    // The output 'safe_path' is considered untainted by the analyzer.
    char safe_path[PATH_MAX];
    if (realpath(input, safe_path) == NULL) {
        return 1;
    }

    // SAFE: path has been sanitized by realpath()
    FILE* f = fopen(safe_path, "r");
    if (f) {
        fclose(f);
    }

    return 0;
}
