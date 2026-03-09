#include "bc_utils.h"
#include "sha256.h"

#include <errno.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int btcp_load_file(nu_buffer_t *buf, const char *filename) {
    FILE *fd = fopen(filename, "rb");
    if (!fd) {
        return errno;
    }

    fseek(fd, 0, SEEK_END);
    long file_size = ftell(fd);
    rewind(fd);

    int ec = 0;

    if (nu_buffer_reserve(buf, file_size)) {
        size_t nb = fread(nu_buffer_begin(buf), 1, file_size, fd);
        if (!nb) {
            ec = errno;
        }
        else {
            nu_buffer_consume(buf, nb);
        }
    }
    else {
        ec = ENOMEM;
    }

    fclose(fd);

    return ec;
}

int btcp_load_text_file(nu_string_t *buf, const char *filename) {
    FILE *fd = fopen(filename, "rt");
    if (!fd) {
        return errno;
    }

    fseek(fd, 0, SEEK_END);
    long file_size = ftell(fd);
    rewind(fd);

    int ec = 0;

    // reserve +1 more
    if (nu_string_reserve(buf, file_size + 1)) {
        size_t nb = fread(nu_string_begin(buf), 1, file_size, fd);
        if (!nb) {
            ec = errno;
        }
        else {
            char *const out = nu_string_consume(buf, nb);
            out[nb] = '\0';
        }
    }
    else {
        ec = ENOMEM;
    }

    fclose(fd);

    return ec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int bc_block_header_hash32(const bc_block_header_t *hdr, bc_hash32_t *hash) {
    return hash256_digest(hdr, sizeof(*hdr), hash->data);
}

int bc_tx_txid(const bc_tx_info_t *tx, bc_hash32_t *hash) {
    nu_buffer_t buf = nu_buffer_create();
    int ec = bc_append_tx_legacy(&buf, tx);
    if (!ec) {
        ec = hash256_digest(nu_dca_data(&buf), nu_dca_size(&buf), hash->data);
    }
    nu_buffer_release(&buf);
    return ec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum {
    PrintOff = 16,
};

void bc_fprint_field_uint8(FILE *fd, const char *tag, uint8_t val) {
    fprintf(fd, "%*s : 0x%02x (%u)\n", PrintOff, tag, val, val);
}

void bc_fprint_field_uint32(FILE *fd, const char *tag, uint32_t val) {
    fprintf(fd, "%*s : 0x%08x (%u)\n", PrintOff, tag, val, val);
}

void bc_fprint_field_uint64(FILE *fd, const char *tag, uint64_t val) {
    fprintf(fd, "%*s : 0x%016lx (%lu)\n", PrintOff, tag, val, val);
}

void bc_fprint_field_varint(FILE *fd, const char *tag, uint64_t val) {
    fprintf(fd, "%*s : 0x%lx (%lu)\n", PrintOff, tag, val, val);
}

void bc_fprint_field_hash32(FILE *fd, const char *tag, const bc_hash32_t *val) {
    bc_hash32_hex_t hash_str = {0};
    bc_hash32_str_make(&hash_str, val);
    fprintf(fd, "%*s : [%s] (str)\n", PrintOff, tag, hash_str.data);
    fprintf(fd, "%*s : [", PrintOff, "");
    for (int i = 0; i < BcHashSize; ++i) {
        printf("%02x", val->data[i]);
    }
    fprintf(fd, "] (hex)\n");
}

void bc_fprint_field_data(FILE *fd, const char *tag, const uint8_t *data,
                          size_t size) {
    fprintf(fd, "%*s : [", PrintOff, tag);
    for (size_t i = 0; i < size; ++i) {
        printf("%02x", data[i]);
    }
    fprintf(fd, "]\n");
}

void bc_fprint_block_header(FILE *fd, const bc_block_header_t *ptr) {
    bc_hash32_t hash = {0};
    bc_block_header_hash32(ptr, &hash);

    fprintf(fd, "- BlockHeader:\n");
    bc_fprint_field_uint32(fd, "version", ptr->version);
    bc_fprint_field_hash32(fd, "prev_block", &ptr->hash_prev_block);
    bc_fprint_field_hash32(fd, "merkle_root", &ptr->hash_merkle_root);
    bc_fprint_field_uint32(fd, "time", ptr->timestamp);
    bc_fprint_field_uint32(fd, "bits", ptr->bits);
    bc_fprint_field_uint32(fd, "nonce", ptr->nonce);
    bc_fprint_field_hash32(fd, "HASH32", &hash);
}

void bc_fprint_tx_input(FILE *fd, const bc_tx_input_t *ptr) {
    fprintf(fd, " - TxInput:\n");
    bc_fprint_field_hash32(fd, "txid", &ptr->txid);
    bc_fprint_field_uint32(fd, "vout", ptr->vout);
    bc_fprint_field_varint(fd, "sig_size", ptr->script_sig_size);
    bc_fprint_field_data(fd, "sig", ptr->script_sig, ptr->script_sig_size);
    bc_fprint_field_uint32(fd, "sequence", ptr->sequence);
}

void bc_fprint_tx_witness(FILE *fd, const bc_witness_t *ptr) {
    fprintf(fd, " - Witness:\n");
    bc_fprint_field_varint(fd, "stack_items", ptr->stack_items);
    for (size_t i = 0; i < ptr->stack_items; ++i) {
        const bc_witness_field_t *f = ptr->items + i;
        bc_fprint_field_varint(fd, "size", f->size);
        bc_fprint_field_data(fd, "item", f->data, f->size);
    }
}

void bc_fprint_tx_output(FILE *fd, const bc_tx_output_t *ptr) {
    fprintf(fd, " - TxOutput:\n");
    bc_fprint_field_uint64(fd, "amount", ptr->amount);
    bc_fprint_field_varint(fd, "pub_key_size", ptr->script_pub_key_size);
    bc_fprint_field_data(fd, "pub_key", ptr->script_pub_key,
                         ptr->script_pub_key_size);
}

void bc_fprint_tx_info(FILE *fd, const bc_tx_info_t *ptr) {
    bc_hash32_t hash = {0};
    bc_tx_txid(ptr, &hash);

    fprintf(fd, "- Transaction\n");
    bc_fprint_field_hash32(fd, "HASH32", &hash);
    bc_fprint_field_uint32(fd, "version", ptr->version);
    bc_fprint_field_uint8(fd, "marker", ptr->marker);
    bc_fprint_field_uint8(fd, "flags", ptr->flags);
    bc_fprint_field_varint(fd, "in_count", ptr->in_count);
    bc_fprint_field_varint(fd, "out_count", ptr->out_count);
    bc_fprint_field_uint32(fd, "lock_time", ptr->lock_time);

    for (uint64_t i = 0; i < ptr->in_count; ++i) {
        bc_fprint_tx_input(fd, ptr->in_vec + i);
        bc_fprint_tx_witness(fd, ptr->witness_vec + i);
    }
    for (uint64_t i = 0; i < ptr->out_count; ++i) {
        bc_fprint_tx_output(fd, ptr->out_vec + i);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
