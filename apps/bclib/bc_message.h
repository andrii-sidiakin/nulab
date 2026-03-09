#ifndef _BC_MESSAGE_H_INCLUDED_
#define _BC_MESSAGE_H_INCLUDED_

#include <bc_utils.h>

#include <nulib/buffer.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    uint8_t magic[4];
    uint8_t command[12];
    uint32_t size;
    uint8_t checksum[4];
} bc_message_header_t;

typedef struct {
    uint64_t services;
    uint8_t ip[16];
    uint16_t port;
} bc_net_addr_t;

typedef struct {
    int32_t version;
    uint64_t service_flags;
    int64_t time;
    bc_net_addr_t addr_recv;
    bc_net_addr_t addr_from;
    uint64_t nonce;
    uint32_t user_agent_size;
    const uint8_t *user_agent_data;
    int32_t start_height;
} bc_version_message_payload_t;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int bc_append_message_header(nu_buffer_t *buf, const bc_message_header_t *ptr) {
    if (nu_buffer_extend(buf, sizeof(*ptr))) {
        bc_append_bytes(buf, ptr->magic, sizeof(ptr->magic));
        bc_append_bytes(buf, ptr->command, sizeof(ptr->command));
        bc_append_u32(buf, ptr->size);
        bc_append_bytes(buf, ptr->checksum, sizeof(ptr->checksum));
        return 0;
    }
    return -1;
}

int bc_append_net_addr(nu_buffer_t *buf, const bc_net_addr_t *ptr) {
    if (nu_buffer_extend(buf, sizeof(*ptr))) {
        bc_append_u64(buf, ptr->services);
        bc_append_bytes(buf, ptr->ip, sizeof(ptr->ip));
        bc_append_u16(buf, ptr->port);
        return 0;
    }
    return -1;
}

int bc_append_version_message_payload(nu_buffer_t *buf,
                                      const bc_version_message_payload_t *ptr) {
    const uint32_t approx_size = sizeof(*ptr) + ptr->user_agent_size;

    if (nu_buffer_extend(buf, approx_size)) {
        bc_append_i32(buf, ptr->version);
        bc_append_u64(buf, ptr->service_flags);
        bc_append_i64(buf, ptr->time);
        bc_append_net_addr(buf, &ptr->addr_recv);
        bc_append_net_addr(buf, &ptr->addr_from);
        bc_append_u64(buf, ptr->nonce);

        bc_append_varint(buf, ptr->user_agent_size);
        if (ptr->user_agent_size > 0) {
            bc_append_bytes(buf, ptr->user_agent_data, ptr->user_agent_size);
        }

        bc_append_i32(buf, ptr->start_height);
        return 0;
    }
    return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
