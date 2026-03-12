#ifndef _BCLIB_CLIENT_H_INCLUDED_
#define _BCLIB_CLIENT_H_INCLUDED_

#include "tcp_client.h"

#include <bc_message.h>

#include <nulib/buffer.h>
#include <stdint.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef enum {
    BcClient_Ok = 0,
    BcClient_Error,
    BcClient_MagicMismatch,
    BcClient_ChecksumMismatch,
    BcClient_TcpError,
} BcClientStatus;

typedef int (*bc_message_callback_t)(void *ctx, const bc_message_header_t *hdr,
                                     const uint8_t *data);

typedef struct {
    nu_tcp_client_t tcp;
    uint8_t magic[4];
    nu_buffer_t sbuf; // send buffer
    nu_buffer_t rbuf; // recv buffer

    BcClientStatus status;

    void *on_msg_ctx;
    bc_message_callback_t on_msg;
} bc_client_t;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int bc_client_connect(bc_client_t *cli, const char *host, const char *port);
int bc_client_disconnect(bc_client_t *cli);

int bc_client_send_version(bc_client_t *cli);
int bc_client_send_verack(bc_client_t *cli);
int bc_client_send_pong(bc_client_t *cli);

int bc_client_verify_version(const bc_client_t *cli,
                             const bc_version_message_payload_t *data);

int bc_client_recv_message_header(bc_client_t *cli, bc_message_header_t *hdr);
int bc_client_next_message(bc_client_t *cli);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
