#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// No tainted sources used at all
int main() {
    const char* safe_cmd = "ls -l /tmp";

    // This is a constant string, not tainted
    system(safe_cmd);  // OK: safe (no taint)

    char buf[256];
    snprintf(buf, sizeof(buf), "echo %s", "hello");
    system(buf);  // OK: safe (no taint sources)

    return 0;
}
