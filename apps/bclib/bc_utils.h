#ifndef _UTILS_H_INCLUDED_
#define _UTILS_H_INCLUDED_

#include "bc_hex.h"
#include "bc_types.h"

#include <nulib/buffer.h>
#include <nulib/string.h>

#include <string.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int btcp_load_file(nu_buffer_t *buf, const char *filename);
int btcp_load_text_file(nu_string_t *buf, const char *filename);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef uint8_t bc_i256_t[32];

static inline int i256_compare(const bc_i256_t lhs, const bc_i256_t rhs) {
    static const size_t N = 32;
    const uint8_t *const L = lhs;
    const uint8_t *const R = rhs;

    for (int i = 0, j = N - 1; i < N; ++i, --j) {
        int c = (int)L[j] - (int)R[j];
        if (c > 0) {
            return +1 + i;
        }
        if (c < 0) {
            return -1 - i;
        }
    }
    return 0;
}

static inline int i256_le(const uint8_t lhs[32], const uint8_t rhs[32]) {
    return i256_compare(lhs, rhs) <= 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static inline int bc_append_varint(nu_buffer_t *buf, uint64_t val) {
    if (val < 0xFD) {
        uint8_t *const out = nu_buffer_consume(buf, 1);
        *out = (uint8_t)val;
        return 0;
    }
    if (val < 0xFFFF) {
        uint8_t *const out = nu_buffer_consume(buf, 3);
        *out = 0xFD;
        *(uint16_t *)(out + 1) = val;
        return 0;
    }
    if (val < 0xFFFF'FFFF) {
        uint8_t *const out = nu_buffer_consume(buf, 5);
        *out = 0xFE;
        *(uint32_t *)(out + 1) = val;
        return 0;
    }

    uint8_t *const out = nu_buffer_consume(buf, 9);
    *out = 0xFF;
    *(uint64_t *)(out + 1) = val;
    return 0;
}

static inline int bc_append_i16(nu_buffer_t *buf, int16_t val) {
    int16_t *const out = (int16_t *)nu_buffer_consume(buf, sizeof(val));
    *out = val;
    return 0;
}

static inline int bc_append_u16(nu_buffer_t *buf, uint16_t val) {
    uint16_t *const out = (uint16_t *)nu_buffer_consume(buf, sizeof(val));
    *out = val;
    return 0;
}

static inline int bc_append_i32(nu_buffer_t *buf, int32_t val) {
    int32_t *const out = (int32_t *)nu_buffer_consume(buf, sizeof(val));
    *out = val;
    return 0;
}

static inline int bc_append_u32(nu_buffer_t *buf, uint32_t val) {
    uint32_t *const out = (uint32_t *)nu_buffer_consume(buf, sizeof(val));
    *out = val;
    return 0;
}

static inline int bc_append_u64(nu_buffer_t *buf, uint64_t val) {
    uint64_t *const out = (uint64_t *)nu_buffer_consume(buf, sizeof(val));
    *out = val;
    return 0;
}

static inline int bc_append_i64(nu_buffer_t *buf, int64_t val) {
    int64_t *const out = (int64_t *)nu_buffer_consume(buf, sizeof(val));
    *out = val;
    return 0;
}

static inline int bc_append_bytes(nu_buffer_t *buf, const uint8_t *dat,
                                  size_t num) {
    uint8_t *const out = nu_buffer_consume(buf, num);
    if (out) {
        memcpy(out, dat, num);
    }
    return out != NULL;
}

static inline int bc_append_hash32(nu_buffer_t *buf, const bc_hash32_t *val) {
    bc_hash32_t *const out =
        (bc_hash32_t *)nu_buffer_consume(buf, sizeof(*val));
    if (out) {
        memcpy(out, val, sizeof(*val));
    }
    return out != NULL;
}

static inline int bc_append_block_header(nu_buffer_t *buf,
                                         const bc_block_header_t *hdr) {
    uint8_t *const out = nu_buffer_consume(buf, sizeof(*hdr));
    if (out) {
        memcpy(out, hdr, sizeof(*hdr));
    }
    return out != NULL;
}

static inline int bc_append_tx_input(nu_buffer_t *buf,
                                     const bc_tx_input_t *txi) {
    bc_append_hash32(buf, &txi->txid);
    bc_append_u32(buf, txi->vout);
    bc_append_varint(buf, txi->script_sig_size);
    const uint64_t sig_sz = txi->script_sig_size;
    if (sig_sz) {
        nu_assert(sig_sz < SIZE_MAX);
        bc_append_bytes(buf, txi->script_sig, txi->script_sig_size);
    }
    bc_append_u32(buf, txi->sequence);
    return 0;
}

static inline int bc_append_tx_output(nu_buffer_t *buf,
                                      const bc_tx_output_t *txo) {
    bc_append_u64(buf, txo->amount);
    bc_append_varint(buf, txo->script_pub_key_size);
    const uint64_t pubkey_sz = txo->script_pub_key_size;
    if (pubkey_sz) {
        nu_assert(pubkey_sz < SIZE_MAX);
        bc_append_bytes(buf, txo->script_pub_key, txo->script_pub_key_size);
    }
    return 0;
}

static inline int bc_append_tx_inputs_section(nu_buffer_t *buf,
                                              const bc_tx_info_t *tx) {
    nu_assert(tx->in_count < SIZE_MAX);
    bc_append_varint(buf, tx->in_count);
    for (size_t i = 0; i < tx->in_count; ++i) {
        bc_append_tx_input(buf, tx->in_vec + i);
    }
    return 0;
}

static inline int bc_append_tx_outputs_section(nu_buffer_t *buf,
                                               const bc_tx_info_t *tx) {
    nu_assert(tx->out_count < SIZE_MAX);
    bc_append_varint(buf, tx->out_count);
    for (size_t i = 0; i < tx->out_count; ++i) {
        bc_append_tx_output(buf, tx->out_vec + i);
    }
    return 0;
}

static inline int bc_append_tx_witnesses_section(nu_buffer_t *buf,
                                                 const bc_tx_info_t *tx) {
    // witness section has the same length as inputs
    nu_assert(tx->in_count < SIZE_MAX);
    bc_append_varint(buf, tx->in_count);
    for (size_t i = 0; i < tx->in_count; ++i) {
        const bc_witness_t *const w = tx->witness_vec + i;
        nu_assert(w->stack_items < SIZE_MAX);
        bc_append_varint(buf, w->stack_items);
        if (w->stack_items) {
            nu_assert(w->items != NULL);
            for (size_t j = 0; j < tx->in_count; ++j) {
                const bc_witness_field_t *const f = w->items + j;
                nu_assert(f->size < SIZE_MAX);
                nu_assert(f->data != NULL);
                bc_append_varint(buf, f->size);
                bc_append_bytes(buf, f->data, f->size);
            }
        }
    }
    return 0;
}

static inline int bc_append_tx_legacy(nu_buffer_t *buf,
                                      const bc_tx_info_t *tx) {
    bc_append_u32(buf, tx->version);
    bc_append_tx_inputs_section(buf, tx);
    bc_append_tx_outputs_section(buf, tx);
    bc_append_u32(buf, tx->lock_time);
    return 0;
}

static inline int bc_append_tx_segwit(nu_buffer_t *buf,
                                      const bc_tx_info_t *tx) {
    bc_append_u32(buf, tx->version);
    bc_append_bytes(buf, &tx->marker, 1);
    bc_append_bytes(buf, &tx->flags, 1);
    bc_append_tx_inputs_section(buf, tx);
    bc_append_tx_outputs_section(buf, tx);
    bc_append_tx_witnesses_section(buf, tx);
    bc_append_u32(buf, tx->lock_time);

    return 0;
}

static inline int bc_append_transaction(nu_buffer_t *buf,
                                        const bc_tx_info_t *tx) {
    if (tx->flags) {
        return bc_append_tx_segwit(buf, tx);
    }
    return bc_append_tx_legacy(buf, tx);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static inline void bc_hash32_from_cspan(bc_hash32_t *h, nu_cspan_t s) {
    nu_assert((s.end - s.beg) >= BcHashStringSize);
    for (size_t i = 0; i < BcHashSize; ++i) {
        h->data[BcHashSize - i - 1] = bc_hex_char_to_byte(s.beg + (i * 2));
    }
}

static inline void bc_hash32_str_make(bc_hash32_hex_t *s,
                                      const bc_hash32_t *h) {
    bc_write_hex_backward(s->data, BcHashStringSize + 1, h->data, BcHashSize);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int bc_block_header_hash32(const bc_block_header_t *hdr, bc_hash32_t *hash);
int bc_tx_txid(const bc_tx_info_t *tx, bc_hash32_t *hash);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void bc_fprint_field_uint8(FILE *fd, const char *tag, uint8_t val);
void bc_fprint_field_uint32(FILE *fd, const char *tag, uint32_t val);
void bc_fprint_field_uint64(FILE *fd, const char *tag, uint64_t val);
void bc_fprint_field_varint(FILE *fd, const char *tag, uint64_t val);
void bc_fprint_field_hash32(FILE *fd, const char *tag, const bc_hash32_t *val);

void bc_fprint_field_data(FILE *fd, const char *tag, const uint8_t *data,
                          size_t size);

void bc_fprint_block_header(FILE *fd, const bc_block_header_t *ptr);
void bc_fprint_tx_input(FILE *fd, const bc_tx_input_t *ptrfd);
void bc_fprint_tx_witness(FILE *fd, const bc_witness_t *ptrfd);
void bc_fprint_tx_output(FILE *fd, const bc_tx_output_t *ptrfd);
void bc_fprint_tx_info(FILE *fd, const bc_tx_info_t *ptr);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
