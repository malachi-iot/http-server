// All references see REFERENCES.md
#include <stdio.h>

#include <err.h>
#include <error.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <logger.h>

// Setting up a memory-only loopback (domain) socket as per [24]


#define SOCK_PATH "pglx_10_unity.server"

int create_domain_socket_client()
{
    int s, t, len;
    struct sockaddr_un remote;
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        //exit(1);
    }

    LOG_DEBUG("Trying to connect...");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCK_PATH);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        //exit(1);
    }

    return s;
}


int create_domain_socket()
{
    unsigned int s;
    struct sockaddr_un local;
    int len;

    s = socket(AF_UNIX, SOCK_STREAM, 0);

    local.sun_family = AF_UNIX;  /* local is declared before socket() ^ */
    //strcpy(local.sun_path, "/home/beej/mysocket");
    strcpy(local.sun_path, SOCK_PATH);

    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family);
    if(bind(s, (struct sockaddr *)&local, len) != 0)
        perror("Can't bind");

    if(listen(s, 5) != 0)
        perror("Can't listen");

    return s;
}

int accept_domain_socket(int domain_fd)
{
    int len = sizeof(struct sockaddr_un);
    union
    {
        struct sockaddr_un remote;
        struct sockaddr generic;
    }   u;
    int fd = accept(domain_fd, &u.generic, &len);

    if(fd == -1) perror("Can't get FD");

    return fd;
}


