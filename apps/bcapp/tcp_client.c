#include "tcp_client.h"

#define _POSIX_C_SOURCE 200809L // Or 200112L
//#define _GNU_SOURCE // Often needed on Linux for full network API access

#include <errno.h>
#include <stdio.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int nu_tcp_client_connect(nu_tcp_client_t *cli, const char *node,
                          const char *service) {
    struct addrinfo hints = {0};
    struct addrinfo *info = NULL;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int ec = getaddrinfo(node, service, &hints, &info);
    if (ec) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ec));
        return ec;
    }

    for (struct addrinfo *addr = info; addr; addr = addr->ai_next) {
        int sd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sd == -1) {
            ec = errno;
            continue;
        }
        if (connect(sd, addr->ai_addr, addr->ai_addrlen) == -1) {
            ec = errno;
            close(sd);
            continue;
        }
        cli->sd = sd;
        break;
    }

    return ec;
}

int nu_tcp_client_disconnect(nu_tcp_client_t *cli) {
    if (cli->sd >= 0) {
        close(cli->sd);
        cli->sd = -1;
    }
    return 0;
}

size_t nu_tcp_client_send(nu_tcp_client_t *cli, const void *msg, size_t len) {
    if (cli->ec || cli->sd < 0) {
        return 0;
    }

    ssize_t ns = send(cli->sd, msg, len, 0); // TODO: flags
    if (ns < 0) {
        cli->ec = errno;
        return 0;
    }
    return ns;
}

size_t nu_tcp_client_recv(nu_tcp_client_t *cli, void *buf, size_t num) {
    if (cli->ec || cli->sd < 0) {
        return 0;
    }

    ssize_t nr = recv(cli->sd, buf, num, 0); // TODO: flags
    if (nr < 0) {
        cli->ec = errno;
        return 0;
    }
    return nr;
}
