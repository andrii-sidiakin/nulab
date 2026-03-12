#include "bc_client.h"
#include "bc_types.h"
#include "nulib/buffer.h"
#include "tcp_client.h"

#include <bc_message.h>
#include <sha256.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

static const int32_t BcClient_ProtocolVersion = 70014;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static int bc_handshake(bc_client_t *cli);

static bc_message_header_t bc_massage_header_make(const bc_client_t *cli,
                                                  const char command[12]);

static bc_message_header_t
bc_massage_header_make_with_data(const bc_client_t *cli, const char command[12],
                                 const uint8_t *data, uint32_t size);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int bc_client_connect(bc_client_t *cli, const char *host, const char *port) {
    cli->status = BcClient_Ok;

    int ec = nu_tcp_client_connect(&cli->tcp, host, port);
    if (ec) {
        cli->status = BcClient_TcpError;
        return ec;
    }

    return bc_handshake(cli);
}

int bc_client_disconnect(bc_client_t *cli) {
    nu_tcp_client_disconnect(&cli->tcp);

    nu_buffer_release(&cli->sbuf);
    nu_buffer_release(&cli->rbuf);

    return 0;
}

int bc_client_send_version(bc_client_t *cli) {
    bc_version_message_payload_t msg_ver_data = {
        .version = BcClient_ProtocolVersion,
        .service_flags = 0,
        .time = time(NULL),
        .addr_recv = {},
        .addr_from = {},
        .nonce = 0,
        .user_agent_size = 0,
        .user_agent_data = NULL,
        .start_height = 0,
    };

    // serialize payload and get its size
    nu_buffer_clean(&cli->sbuf);
    bc_append_version_message_payload(&cli->sbuf, &msg_ver_data);
    const uint32_t data_size = nu_buffer_size(&cli->sbuf);

    bc_message_header_t header = bc_massage_header_make_with_data(
        cli, "version", nu_buffer_data(&cli->sbuf), data_size);

    nu_tcp_client_send(&cli->tcp, &header, sizeof(header));
    if (cli->tcp.ec) {
        cli->status = BcClient_TcpError;
        return 1;
    }

    nu_tcp_client_send(&cli->tcp, nu_buffer_data(&cli->sbuf),
                       nu_buffer_size(&cli->sbuf));
    if (cli->tcp.ec) {
        cli->status = BcClient_TcpError;
        return 1;
    }

    return cli->status == BcClient_Ok ? 0 : 1;
}

int bc_client_send_verack(bc_client_t *cli) {
    bc_message_header_t hdr = bc_massage_header_make(cli, "verack");
    nu_tcp_client_send(&cli->tcp, &hdr, sizeof(hdr));
    return cli->tcp.ec;
}

int bc_client_send_pong(bc_client_t *cli) {
    bc_message_header_t hdr = bc_massage_header_make(cli, "pong");
    nu_tcp_client_send(&cli->tcp, &hdr, sizeof(hdr));
    return cli->tcp.ec;
}

int bc_client_verify_version(const bc_client_t *cli,
                             const bc_version_message_payload_t *data) {
    if (data->version < BcClient_ProtocolVersion) {
        return 1;
    }

    return 0;
}

int bc_client_recv_message_header(bc_client_t *cli, bc_message_header_t *hdr) {
    const size_t M = sizeof(hdr->magic);
    size_t nr = 0;

    nr = nu_tcp_client_recv(&cli->tcp, hdr->magic, M);
    if (nr != M) {
        cli->status = BcClient_TcpError;
        return 1;
    }

    if (memcmp(hdr->magic, cli->magic, M) != 0) {
        cli->status = BcClient_MagicMismatch;
        return 1;
    }

    nr = nu_tcp_client_recv(&cli->tcp, (uint8_t *)hdr + 4, sizeof(*hdr) - 4);
    if (nr != sizeof(*hdr) - 4) {
        cli->status = BcClient_TcpError;
        return 1;
    }

    return 0;
}

int bc_client_next_message(bc_client_t *cli) {
    bc_message_header_t hdr = {0};
    uint8_t *data = NULL;

    int ec = bc_client_recv_message_header(cli, &hdr);
    if (ec) {
        return ec;
    }

    nu_buffer_clean(&cli->sbuf);
    if (hdr.size) {
        data = nu_buffer_consume(&cli->sbuf, hdr.size);
        size_t n = nu_tcp_client_recv(&cli->tcp, data, hdr.size);
        if (n != hdr.size) {
            cli->status = BcClient_TcpError;
            return 1;
        }
    }

    if (cli->on_msg) {
        ec = cli->on_msg(cli->on_msg_ctx, &hdr, data);
    }

    return ec;
}

static int bc_handshake(bc_client_t *cli) {
    int ec = 0;

    ec = bc_client_send_version(cli);

    return ec;
}

static bc_message_header_t bc_massage_header_make(const bc_client_t *cli,
                                                  const char command[12]) {
    return bc_massage_header_make_with_data(cli, command, NULL, 0);
}

static bc_message_header_t
bc_massage_header_make_with_data(const bc_client_t *cli, const char command[12],
                                 const uint8_t *data, uint32_t size) {
    uint8_t hash[BcHashSize] = {0};
    hash256_digest(data, size, hash);

    bc_message_header_t header = {0};
    memcpy(header.magic, cli->magic, sizeof(header.magic));
    strncpy((char *)header.command, command, sizeof(header.command) - 1);
    header.size = size;
    memcpy(header.checksum, hash, sizeof(header.checksum));

    return header;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
