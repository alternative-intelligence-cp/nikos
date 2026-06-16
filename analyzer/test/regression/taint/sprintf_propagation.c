#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char* getenv(const char* name);

int main() {
    char* user_input = getenv("USER_INPUT");
    if (!user_input) {
        return 0;
    }

    char buf[256];
    sprintf(buf, "echo %s", user_input);

    // buf is tainted via sprintf from tainted source
    system(buf);  // ERROR: command injection

    return 0;
}
