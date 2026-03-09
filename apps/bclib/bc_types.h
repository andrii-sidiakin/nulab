#ifndef _BC_TYPES_H_INCLUDED_
#define _BC_TYPES_H_INCLUDED_

#include <nulib/dca.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum {
    BcHashSize = 32,
    BcHashStringSize = BcHashSize * 2,

    BcTxDataSizeMin = 32,
    BcTxDataStringSizeMin = BcTxDataSizeMin * 2,

    BcBlockHeaderSize = 80,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    uint8_t data[BcHashSize];
} bc_hash32_t;

typedef struct {
    char data[BcHashStringSize + 1];
} bc_hash32_hex_t;

typedef struct {
    uint32_t version;
    bc_hash32_t hash_prev_block;  // char[32]
    bc_hash32_t hash_merkle_root; // char[32]
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
} bc_block_header_t;

typedef struct {
    bc_hash32_t txid;
    uint32_t vout;
    uint64_t script_sig_size;
    const uint8_t *script_sig;
    uint32_t sequence;
} bc_tx_input_t;

typedef struct {
    uint64_t amount;
    uint64_t script_pub_key_size;
    const uint8_t *script_pub_key;
} bc_tx_output_t;

typedef struct {
    uint64_t size;
    const uint8_t *data;
} bc_witness_field_t;

typedef struct {
    uint64_t stack_items;
    const bc_witness_field_t *items;
} bc_witness_t;

typedef struct {
    uint32_t version;
    uint64_t in_count;
    const bc_tx_input_t *in_vec;
    uint64_t out_count;
    const bc_tx_output_t *out_vec;
    uint32_t lock_time;

    uint8_t marker;
    uint8_t flags;
    const bc_witness_t *witness_vec; // same length as in_vec
} bc_tx_info_t;

typedef nu_G_dca(bc_hash32_t) bc_hash32_array_t;
typedef nu_G_dca(bc_tx_input_t) bc_tx_input_array_t;
typedef nu_G_dca(bc_tx_output_t) bc_tx_output_array_t;
typedef nu_G_dca(bc_witness_field_t) bc_witness_field_array_t;

static_assert(sizeof(bc_hash32_t) == BcHashSize);
static_assert(sizeof(bc_hash32_hex_t) == BcHashStringSize + 1);
static_assert(sizeof(bc_block_header_t) == BcBlockHeaderSize);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
