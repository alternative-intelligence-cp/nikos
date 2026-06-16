#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main() {
    char buf[256];
    
    // read() from stdin — always tainted
    ssize_t n = read(0, buf, sizeof(buf) - 1);
    if (n <= 0) {
        return 0;
    }
    buf[n] = '\0';

    system(buf);  // ERROR: command injection (from read)

    return 0;
}
