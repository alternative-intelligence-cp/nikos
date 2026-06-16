/*
 * Multi-sink test: single tainted source reaches multiple distinct sink types.
 *
 * Expected: UNSAFE (CommandInjection + PathTraversal + FormatString)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    char cmd[256];
    char path[256];
    const char* input = getenv("USER_INPUT");
    if (!input) return 1;

    /* Sink 1: CommandInjection */
    snprintf(cmd, sizeof(cmd), "process %s", input);
    system(cmd); /* UNSAFE: CommandInjection */

    /* Sink 2: PathTraversal */
    snprintf(path, sizeof(path), "/tmp/%s.log", input);
    FILE* f = fopen(path, "w"); /* UNSAFE: PathTraversal */
    if (f) fclose(f);

    /* Sink 3: FormatString */
    printf(input); /* UNSAFE: FormatString */

    return 0;
}
