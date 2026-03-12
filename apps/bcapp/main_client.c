#include "bc_client.h"

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

static const uint8_t MainNetMagic[] = {0xF9, 0xBE, 0xB4, 0xD9};
static const uint8_t TestNetMagic[] = {0x0B, 0x11, 0x09, 0x07};
static const uint8_t TestNet4Magic[] = {0x1C, 0x16, 0x3F, 0x28};
static const uint8_t RegTestMagic[] = {0xFA, 0xBF, 0xB5, 0xDA};

static const char NodeHost[] = "192.168.0.105";
static const char MainNetPort[] = "8333";
static const char TestNetPort[] = "18333";
static const char TestNet4Port[] = "48333";
// static const char RegTestPort[] = "18334";

int on_msg(void *ctx, const bc_message_header_t *hdr, const uint8_t *data);
int on_inv(void *ctx, const uint8_t *data, uint32_t size);

int on_msg(void *ctx, const bc_message_header_t *hdr, const uint8_t *data) {
    const char *cmd = (const char *)hdr->command;
    const size_t cmd_max = sizeof(hdr->command);

    bc_client_t *const cli = ctx;

    if (memcmp(hdr->magic, cli->magic, sizeof(cli->magic)) != 0) {
        fprintf(stderr, ":e: magic mismatch\n");
        return 1;
    }

    uint8_t hash[BcHashSize] = {0};
    hash256_digest(data, hdr->size, hash);
    if (memcmp(hdr->checksum, hash, 4) != 0) {
        fprintf(stderr, ":e: checksum mismatch\n");
        return 1;
    }

    if (strncmp(cmd, "version", cmd_max) == 0) {
        const bc_version_message_payload_t *ver_data =
            (const bc_version_message_payload_t *)data;
        if (bc_client_verify_version(cli, ver_data) == 0) {
            fprintf(stdout, ":d: on 'version' do 'verack'\n");
            bc_client_send_verack(cli);
        }
        return 0;
    }

    if (strncmp(cmd, "verack", cmd_max) == 0) {
        fprintf(stdout, ":d: on 'verack' do NOTHING\n");
        return 0;
    }

    if (strncmp(cmd, "ping", cmd_max) == 0) {
        fprintf(stdout, ":d: on 'ping' do 'pong'\n");
        bc_client_send_pong(cli);
        return 0;
    }

    if (strncmp(cmd, "inv", cmd_max) == 0) {
        fprintf(stdout, ":d: on 'inv'\n");
        return on_inv(ctx, data, hdr->size);
    }

    if (strncmp(cmd, "sendcmpct", cmd_max) == 0) {
        fprintf(stdout, ":d: on 'sendcmpct' do NOTHING\n");
        fprintf(stdout, "... [%u][%lu]\n", *data, *(uint64_t *)(data + 1));
        return 0;
    }

    if (strncmp(cmd, "feefilter", cmd_max) == 0) {
        fprintf(stdout, ":d: on 'feefilter' do NOTHING\n");
        return 0;
    }

    fprintf(stdout,
            ":e: unknown header { magic=[%02x%02x%02x%02x], command=[%.12s], "
            "payload_size=%u, "
            "checksum=[%02x%02x%02x%02x] }\n",
            hdr->magic[0], hdr->magic[1], hdr->magic[2], hdr->magic[3],
            hdr->command, hdr->size, hdr->checksum[0], hdr->checksum[1],
            hdr->checksum[2], hdr->checksum[3]);

    return 1;
}

int on_inv(void *ctx, const uint8_t *data, uint32_t size) {
    if (nu_expect(size > 0 && data != NULL)) {
        return 1;
    }

    uint8_t nids = *(data);

    for (uint32_t i = 0; i < nids; ++i) {
        const uint8_t *const inv = data + (i * 36) + 1;
        uint32_t tid = *(uint32_t *)(inv);
        if (tid != 1) {
            printf("... inv: {tid=0x%0x}, ...\n", tid);
            bc_fprint_field_data(stdout, "inv_hash", inv + 4, 32);
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    //
    // const char *Port = MainNetPort;
    // const uint8_t *Magic = MainNetMagic;
    //
    // const char *Port = TestNetPort;
    // const uint8_t *Magic = TestNetMagic;
    //
    const char *Port = TestNet4Port;
    const uint8_t *Magic = TestNet4Magic;

    options_t opts = {
        .host = NodeHost,
        .port = Port,
    };

    if (parse_args(&opts, argc, argv)) {
        return -1;
    }

    bc_client_t cli = {
        .tcp = {0},
        .sbuf = nu_buffer_create(),
        .rbuf = nu_buffer_create(),
        .on_msg_ctx = &cli,
        .on_msg = on_msg,
    };

    memcpy(cli.magic, Magic, 4);

    printf("connecting to [%s:%s]...\n", opts.host, opts.port);
    if (bc_client_connect(&cli, opts.host, opts.port)) {
        goto done;
    }

    int num_fails = 0;
    bc_message_header_t hdr = {0};
    while (cli.tcp.ec == 0) {
        if (bc_client_next_message(&cli)) {
            if (cli.status == BcClient_TcpError) {
                perror("read message");
                break;
            }
            num_fails += 1;
        }
        else {
            num_fails = 0;
        }

        if (num_fails > 10) {
            printf("failed to read next message %d times.\n", num_fails);
            break;
        }
    }

#if 0
    bc_hash32_t hash = {0};
    bc_message_header_t header = {0};

    uint64_t num_fails = 0;

    while (cli.tcp.ec == 0) {
        if (bc_client_recv_message_header(&cli, &header)) {
            num_fails += 1;
            continue;
        }

        if (num_fails) {
            printf("!!! failed to read header %lu times\n", num_fails);
            num_fails = 0;
        }

        nu_buffer_clean(&cli.rbuf);
        if (header.size) {
            uint8_t *const buf = nu_buffer_consume(&cli.rbuf, header.size);

            printf("... payload: '%s', %u bytes\n", header.command,
                   header.size);

            nu_tcp_client_recv(&cli.tcp, buf, header.size);
        }

        if (strcmp((const char *)header.command, "ping") == 0) {
            uint8_t *const buf = nu_buffer_begin(&cli.rbuf);
            memcpy(buf, "pong", 4);
            hash256_digest(buf, header.size, hash.data);
            memcpy(header.checksum, hash.data, 4);
            nu_tcp_client_send(&cli.tcp, &header, header.size);
            nu_tcp_client_send(&cli.tcp, buf, header.size);

            continue;
        }

        if (strcmp((const char *)header.command, "sendcmpct") == 0) {
            const uint8_t *const buf = nu_buffer_data(&cli.rbuf);
            printf("... [%u][%lu]\n", *buf, *(uint64_t *)(buf + 1));

            continue;
        }

        if (strcmp((const char *)header.command, "inv") == 0) {
            const uint8_t *const buf = nu_buffer_data(&cli.rbuf);
            uint8_t nids = *(buf);
            for (uint32_t i = 0; i < nids; ++i) {
                const uint8_t *const inv = buf + (i * 36) + 1;
                uint32_t tid = *(uint32_t *)(inv);
                if (tid != 1) {
                    printf("... inv: {tid=0x%0x}, ...\n", tid);
                    bc_fprint_field_data(stdout, "inv_hash", inv + 4, 32);
                }
            }

            continue;
        }

        printf("!!! Ignoted: { magic=[%02x%02x%02x%02x], command=[%s], "
               "payload_size=%u, "
               "checksum=[%02x%02x%02x%02x] }\n",
               header.magic[0], header.magic[1], header.magic[2],
               header.magic[3], header.command, header.size, header.checksum[0],
               header.checksum[1], header.checksum[2], header.checksum[3]);
    }
#endif

done:
    if (cli.tcp.ec) {
        printf("error: %s\n", strerror(cli.tcp.ec));
    }

    printf("disconnecting...\n");
    bc_client_disconnect(&cli);

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
