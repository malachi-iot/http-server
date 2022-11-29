// All references see README.md
#include <stdio.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <signal.h>

#include <logger.h>

#include "bind.h"
#include "threadpool.h"

#include <http/pipeline.h>
#include "http/transport.h"

typedef struct
{
    int connfd;

    HttpPipeline* pipeline;

} ClientAppearedMessage;


void* client_appeared(void* arg)
{
    SharedPtr* p = arg;
    ClientAppearedMessage* m = p->value;
    HttpTransport transport;

    // DEBT: a placement new would be better
    transport.connfd = m->connfd;
    transport.type = HTTP_TRANSPORT_SOCKET;

    http_pipeline_handle_incoming_request(m->pipeline, &transport);

    // NOTE: Looks like fclose inside handler closes underlying connection too
    close(m->connfd);

    shared_ptr_delete(p);

    return arg;
}

extern volatile sig_atomic_t signalShutdown;

static void http_threadpool_tcp_listener(ThreadPool* thread_pool, HttpPipeline* pipeline, int port)
{
    int listenfd = create_listen_ipv4_socket(port, 20);

    assert(("listen error", listenfd >= 0));

    // non-blocking socket, so that select and accept can interoperate w/o race condition
    fcntl(listenfd, F_SETFL, O_NONBLOCK);

    printf("Listening on port %d\n", port);

    while (!signalShutdown) {
        // Guidance from [20.2] semi-spin wait to accept an incoming socket
        struct timeval timeout;
        int rv;
        fd_set set;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        FD_ZERO(&set); /* clear the set */
        FD_SET(listenfd, &set); /* add our file descriptor to the set */

        // Wait for 5s for activity on this socket.
        // DEBT: Not sure what the listenfd + 1 is for, but it is needed.  Otherwise
        // this never seems to get accepts
        rv = select(listenfd + 1, &set, NULL, NULL, &timeout);

        if(rv == 0)
        {
            // Reach here when timed out or sigint'd - accept does not appear to
            // get sigint'd
            static int counter = 0;
            LOG_TRACE("http_threadpool_tcp_listener: counter %d", ++counter);
            continue;
        }
        else if(rv < 0)
        {
            // Iterate through another loop since by now signalShutdown
            // would be set, and if it was a different error we might just try again
            // Also, it turns out GDB and friends trigger a kind of sigint as per [22]
            warn("couldn't select");
            continue;
        }

        SharedPtr* p = shared_ptr_new(NULL, sizeof(ClientAppearedMessage));
        ClientAppearedMessage* cam = p->value;

        cam->connfd = accept(listenfd, NULL, NULL);
        cam->pipeline = pipeline;
        if (cam->connfd < 0) {
            warn("accept error");
            shared_ptr_delete(p);
            return;
        }

        // DEBT: Set back to blocking, though the code is designed to work in a non blocking
        // capacity.  In the short term OK that it is blocking
        fcntl(cam->connfd, F_SETFL, 0);

        LOG_TRACE("http_threadpool_tcp_listener: accepted");

        threadpool_queue(thread_pool, client_appeared, p);
    }
}

void http_pipeline_threadpool_tcp_listener(ThreadPool* thread_pool, HttpPipeline* pipeline, int port)
{
    http_threadpool_tcp_listener(thread_pool, pipeline, port);
}

