#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern char* getenv(const char* name);

int main() {
    char* user_input = getenv("USER_INPUT");
    if (!user_input) {
        return 0;
    }
    
    char command[256];
    snprintf(command, sizeof(command), "ls -l %s", user_input);

    system(command);

    return 0;
}
