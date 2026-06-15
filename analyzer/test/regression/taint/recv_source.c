// recv_source.c
//
// Taint test: recv() as a source.
// Data received over a network socket is untrusted (tainted).
// When passed directly to system(), this is a command injection.
//
// Expected: error (CommandInjection)
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int main(int argc, char* argv[]) {
    int sockfd = 0; // assume pre-opened socket fd

    char cmd[256];
    ssize_t n = recv(sockfd, cmd, sizeof(cmd) - 1, 0);
    if (n <= 0) {
        return 1;
    }
    cmd[n] = '\0';

    // ERROR: network-received data flows directly to system()
    system(cmd);

    return 0;
}
