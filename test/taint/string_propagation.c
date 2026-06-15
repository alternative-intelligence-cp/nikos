#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char* getenv(const char* name);

int main() {
    char* user_input = getenv("USER_INPUT");
    if (!user_input) {
        return 0;
    }

    // Test 1: snprintf propagation (known working)
    char cmd1[256];
    snprintf(cmd1, sizeof(cmd1), "echo %s", user_input);
    system(cmd1);  // ERROR: command injection via snprintf

    // Test 2: sprintf propagation
    char cmd2[256];
    sprintf(cmd2, "ls %s", user_input);
    system(cmd2);  // ERROR: command injection via sprintf

    return 0;
}
