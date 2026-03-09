#include "tcp_client.h"

#include <nulib/buffer.h>
#include <nulib/string.h>

#include <bc_message.h>
#include <bc_types.h>
#include <bc_utils.h>
#include <sha256.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    nu_tcp_client_t tcp;
    nu_buffer_t sbuf; // send buffer
    nu_buffer_t rbuf; // recv buffer
} bc_client_t;

int bc_client_recv_message_header(bc_client_t *cli, bc_message_header_t *hdr) {
    printf("wating for message header...\n");
    size_t nr = nu_tcp_client_recv(&cli->tcp, hdr, sizeof(*hdr));

    if (nr == sizeof(*hdr)) {
        printf("... { magic=[%02x%02x%02x%02x], command=[%s], payload_size=%u, "
               "checksum=[%02x%02x%02x%02x] }\n",
               hdr->magic[0], hdr->magic[1], hdr->magic[2], hdr->magic[3],
               hdr->command, hdr->size, hdr->checksum[0], hdr->checksum[1],
               hdr->checksum[2], hdr->checksum[3]);

        return 0;
    }

    return -1;
}

int bc_client_recv_version_message(bc_client_t *cli,
                                   const bc_message_header_t *hdr,
                                   bc_version_message_payload_t *msg) {

    nu_buffer_reserve(&cli->rbuf, hdr->size);

    uint8_t *const buf = nu_buffer_begin(&cli->rbuf);

    printf("waiting for version_message_payload...\n");
    size_t nr = nu_tcp_client_recv(&cli->tcp, buf, hdr->size);

    if (nr == hdr->size) {
        size_t c1 = offsetof(bc_version_message_payload_t, user_agent_size);

        // copy data up to the user_agent_size field
        memcpy(msg, buf, c1);

        uint32_t ua_sz = (buf + c1)[0]; // TODO: parse compact_size

        msg->user_agent_size = ua_sz;
        msg->user_agent_data = NULL;
        msg->start_height = *(int32_t *)(buf + c1 + 1);

        printf("... version: %d\n", msg->version);
        printf("... time: %ld\n", msg->time);
        printf("... nonce: %ld\n", msg->nonce);
        printf("... user_agent_size: %d\n", msg->user_agent_size);
        printf("... start_height: %d\n", msg->start_height);
    }

    return nr == hdr->size ? 0 : -1;
}

int bc_client_connect(bc_client_t *cli, const char *host, const char *port) {
    int ec = nu_tcp_client_connect(&cli->tcp, host, port);
    if (ec) {
        return ec;
    }
    return ec;
}

int bc_handshake(bc_client_t *cli) {
    bc_version_message_payload_t version_payload = {
        .version = 70014,
        .service_flags = 0,
        .time = time(NULL),
        .addr_recv = {},
        .addr_from = {},
        .nonce = 0,
        .user_agent_size = 0,
        .user_agent_data = NULL,
        .start_height = 939955,
    };

    bc_message_header_t version_header = {
        .magic = {0xf9, 0xbe, 0xb4, 0xd9},
        .command = "version",
        .size = 0,
        .checksum = "",
    };

    bc_message_header_t verack_header = {
        .magic = {0xf9, 0xbe, 0xb4, 0xd9},
        .command = "verack",
        .size = 0,
        .checksum = "",
    };

    bc_hash32_t payload_hash = {0};
    bc_message_header_t header = {0};

    // make version message
    nu_buffer_clean(&cli->sbuf);
    bc_append_version_message_payload(&cli->sbuf, &version_payload);
    version_payload = (bc_version_message_payload_t){0};

    const uint32_t payload_size = nu_buffer_size(&cli->sbuf);
    hash256_digest(nu_buffer_data(&cli->sbuf), payload_size, payload_hash.data);
    version_header.size = payload_size;
    memcpy(version_header.checksum, payload_hash.data, 4);

    // send version message

    nu_tcp_client_send(&cli->tcp, &version_header, sizeof(version_header));
    nu_tcp_client_send(&cli->tcp, nu_buffer_data(&cli->sbuf),
                       nu_buffer_size(&cli->sbuf));

    // recv version message

    bc_client_recv_message_header(cli, &header);
    if (strcmp((const char *)header.command, "version") != 0) {
        return -1;
    }
    bc_client_recv_version_message(cli, &header, &version_payload);
    // TODO: verify version_payload

    // recv verack message
    bc_client_recv_message_header(cli, &header);
    if (strcmp((const char *)header.command, "verack") != 0) {
        return -1;
    }

    // make/send verack
    hash256_digest("", 0, payload_hash.data);
    memcpy(verack_header.checksum, payload_hash.data, 4);
    nu_tcp_client_send(&cli->tcp, &verack_header, sizeof(verack_header));

    return cli->tcp.ec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    int argc;
    char **argv;
} args_t;

typedef struct {
    const char *app;

    const char *host;
    const char *port;
} options_t;

int parse_args(options_t *opts, int argc, char *argv[]);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static const uint8_t MainNetMagic[] = {0XF9, 0XBE, 0XB4, 0XD9};
static const uint8_t TestNetMagic[] = {0xFA, 0xBF, 0xB5, 0xDA};

static const char NodeHost[] = "192.168.0.105";
static const char MainNetPort[] = "8333";
static const char TestNetPort[] = "8332";

int main(int argc, char *argv[]) {
    options_t opts = {
        .host = NodeHost,
        .port = MainNetPort,
    };

    if (parse_args(&opts, argc, argv)) {
        return -1;
    }

    bc_client_t cli = {
        .tcp = {0},
        .sbuf = nu_buffer_create(),
        .rbuf = nu_buffer_create(),
    };

    printf("connecting to [%s:%s]...\n", opts.host, opts.port);
    if (bc_client_connect(&cli, opts.host, opts.port)) {
        goto done;
    }

    printf("handshake...\n");
    if (bc_handshake(&cli)) {
        goto done;
    }

    bc_hash32_t hash = {0};
    bc_message_header_t header = {0};

    while (cli.tcp.ec == 0) {
        if (bc_client_recv_message_header(&cli, &header)) {
            goto done;
        }

        if (memcmp(header.magic, MainNetMagic, 4) != 0) {
            printf("!!! magic mismatch\n");
            continue;
        }

        if (header.size) {
            nu_buffer_clean(&cli.rbuf);
            uint8_t *const buf = nu_buffer_consume(&cli.rbuf, header.size);
            printf("... payload: '%s', %u bytes\n", header.command,
                   header.size);
            nu_tcp_client_recv(&cli.tcp, buf, header.size);

            if (strcmp((const char *)header.command, "ping") == 0) {
                memcpy(buf, "pong", 4);
                hash256_digest(buf, header.size, hash.data);
                memcpy(header.checksum, hash.data, 4);
                nu_tcp_client_send(&cli.tcp, &header, header.size);
                nu_tcp_client_send(&cli.tcp, buf, header.size);

                continue;
            }

            if (strcmp((const char *)header.command, "sendcmpct") == 0) {
                printf("... [%u][%lu]\n", *buf, *(uint64_t *)(buf + 1));

                continue;
            }

            printf("... skipped\n");
            continue;
        }

        printf("break the loop\n");
        break;
    }

done:
    if (cli.tcp.ec) {
        printf("error: %s\n", strerror(cli.tcp.ec));
    }

    printf("disconnecting...\n");
    nu_tcp_client_disconnect(&cli.tcp);

    nu_buffer_release(&cli.sbuf);
    nu_buffer_release(&cli.rbuf);

    printf("done.\n");
    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int parse_opt_with_value(const args_t *args, const char *key,
                         const char **val) {
    for (int i = 0; i < args->argc; ++i) {
        if (strcmp(args->argv[i], key) == 0) {
            if (i + 1 < args->argc) {
                *val = args->argv[i];
                return 0; // success
            }
            return -2; // no value for key
        }
    }
    return -1; // key not found
}

int parse_args(options_t *opts, int argc, char *argv[]) {
    int ec = 0;
    if (argc < 1) {
        return -1;
    }

    const args_t args = {argc - 1, argv + 1};

    opts->app = argv[0];

    return ec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
