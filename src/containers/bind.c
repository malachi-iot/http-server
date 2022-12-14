#include <assert.h>

#include <signal.h>
#include <string.h>

#include <arpa/inet.h>

#include "bind.h"

#if ESP_PLATFORM
#include <lwip/sockets.h>
#endif

int create_listen_ipv4_socket(uint16_t port, int backlog) {
#if !ESP_PLATFORM
    signal(SIGPIPE, SIG_IGN);
#endif
    assert(("port must be > 0", port > 0));

    struct sockaddr_in addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        return -2;
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htons(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        return -3;
    }
    if (listen(listenfd, backlog) < 0) {
        return -4;
    }
    return listenfd;
}
