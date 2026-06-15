// snprintf_sink.c
//
// Taint test: getenv() → snprintf() intermediate → system() sink.
// Taint propagates through snprintf() into the output buffer, because
// snprintf() is NOT a sanitizer (it's a format/copy function, not a
// security boundary). The tainted buffer then reaches system().
//
// This tests that the analyzer correctly tracks taint through intermediate
// string-formatting operations.
//
// Expected: error (CommandInjection)
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
    const char* user_input = getenv("CMD_ARGS");
    if (!user_input) {
        return 0;
    }

    // snprintf is NOT a sanitizer: it copies/formats the tainted string
    // into 'cmd', so 'cmd' is also tainted.
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "process --arg=%s", user_input);

    // ERROR: cmd is tainted (derived from getenv) via snprintf
    system(cmd);

    return 0;
}
