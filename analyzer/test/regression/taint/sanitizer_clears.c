#include <stdlib.h>
#include <string.h>

extern char* getenv(const char* name);
extern char* escape_shell_arg(const char* arg);

int main() {
    char* user_input = getenv("USER_INPUT");
    if (!user_input) {
        return 0;
    }

    // Sanitize the input
    char* safe = escape_shell_arg(user_input);

    // safe should be untainted after sanitizer
    system(safe);  // OK: safe (sanitized)

    return 0;
}
