#ifndef _NULIB_TCP_H_INCLUDED_
#define _NULIB_TCP_H_INCLUDED_

#include <stddef.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    int sd;
    int ec;
} nu_tcp_client_t;

int nu_tcp_client_connect(nu_tcp_client_t *cli, const char *node,
                          const char *service);

int nu_tcp_client_disconnect(nu_tcp_client_t *cli);

size_t nu_tcp_client_send(nu_tcp_client_t *cli, const void *msg, size_t len);

size_t nu_tcp_client_recv(nu_tcp_client_t *cli, void *buf, size_t num);

#endif
