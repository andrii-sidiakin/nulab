#include <bc_json.h>
#include <bc_merkle_root.h>
#include <bc_types.h>
#include <bc_utils.h>

#include <nulib/dca.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef nu_G_dca(bc_tx_view_t) tx_view_dca_t;

static int tx_view_dca_append(tx_view_dca_t *txs, const bc_tx_view_t *tx) {
    bc_tx_view_t *const out = nu_dca_consume(txs, 1);
    if (out) {
        *out = *tx;
    }
    return out == NULL;
}

static int read_txs_from_json(tx_view_dca_t *txs, nu_cspan_t json) {
    bc_tx_scan_ctx_t scan_ctx = {
        .cb = (bc_tx_scan_callback_t)tx_view_dca_append,
        .user_ptr = txs,
    };
    return json_blocktemplate_tx_scan(json, &scan_ctx);
}

int main(int argc, const char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <path-to-blocktemaplte.json>\n", argv[0]);
        return -1;
    }

    nu_string_t blocktemplate_json_str = nu_string_create();
    if (btcp_load_text_file(&blocktemplate_json_str, argv[1])) {
        fprintf(stderr, "Failed to read file '%s'\n", argv[1]);
        nu_dca_release(&blocktemplate_json_str);
        return -1;
    }

    const nu_cspan_t json = nu_cspan_from_string(&blocktemplate_json_str);

    tx_view_dca_t tx_arr = {.mem = nu_memory_get_default()};
    if (read_txs_from_json(&tx_arr, json)) {
        fprintf(stderr, "Failed to scan transactions\n");
        nu_dca_release(&blocktemplate_json_str);
        nu_dca_release(&tx_arr);
        return -1;
    }

    const size_t tx_count = nu_dca_size(&tx_arr);
    bc_hash32_array_t txid_arr = {.mem = nu_memory_get_default()};
    bc_hash32_t *const txid_ptr = nu_dca_consume(&txid_arr, tx_count);

    for (size_t i = 0; i < tx_count; ++i) {
        const bc_tx_view_t *tx_ptr = nu_dca_data(&tx_arr) + i;
        bc_hash32_from_cspan(txid_ptr + i, tx_ptr->hash);
        // tx_view_print(tx);
    }

    bc_hash32_array_t mr_tmp_arr = {.mem = nu_memory_get_default()};
    bc_merkle_root(&txid_arr, &mr_tmp_arr);

    bc_hash32_t merkle_root = nu_dca_data(&mr_tmp_arr)[0];
    nu_dca_release(&mr_tmp_arr);

    const nu_cspan_t target_data = json_find_target(json);
    const nu_cspan_t prev_block_data = json_find_prevblock(json);
    const nu_cspan_t bits_data = json_find_bits(json);
    const nu_cspan_t mintime_data = json_find_mintime(json);
    const nu_cspan_t height_data = json_find_height(json);

    uint32_t height = strtoul(height_data.beg, NULL, 10);
    uint32_t mintime = strtoul(mintime_data.beg, NULL, 10);
    uint32_t bits = strtoul(bits_data.beg, NULL, 16);

    bc_hash32_t target = {0};
    bc_hash32_t prev_block = {0};
    bc_hash32_from_cspan(&target, target_data);
    bc_hash32_from_cspan(&prev_block, prev_block_data);

    fprintf(stdout, "Template Data (from: '%s')\n", argv[1]);
    bc_fprint_field_uint32(stdout, "height", height);
    bc_fprint_field_uint32(stdout, "mintime", mintime);
    bc_fprint_field_uint32(stdout, "bits", bits);
    bc_fprint_field_hash32(stdout, "target", &target);
    bc_fprint_field_hash32(stdout, "prev block hash", &prev_block);

    fprintf(stdout, "Augmented Data:\n");
    bc_fprint_field_uint32(stdout, "tx count", tx_count);
    bc_fprint_field_hash32(stdout, "merkle root", &merkle_root);

    bc_block_header_t block = {0};
    block.version = 0x20000000;
    block.hash_prev_block = prev_block;
    block.hash_merkle_root = merkle_root;
    block.timestamp = mintime;
    block.bits = bits;
    block.nonce = 0;
    bc_fprint_block_header(stdout, &block);

    nu_dca_release(&tx_arr);
    nu_dca_release(&txid_arr);
    nu_string_release(&blocktemplate_json_str);

    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
