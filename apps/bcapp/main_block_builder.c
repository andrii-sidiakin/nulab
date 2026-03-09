#include <bc_types.h>
#include <bc_utils.h>

#include <nulib/buffer.h>
#include <nulib/dca.h>
#include <nulib/memory.h>
#include <nulib/string.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

static const uint32_t BcBlockVersion_Min = 0x04;
static const uint32_t BcBlockVersion_Max = 0x7FFFFFFF;
static const uint32_t BcBlockVersion_Default = 0x20000000;

static const uint32_t BcTxVersion_Basic = 0x01;
static const uint32_t BcTxVersion_HeightInCoinbase = 0x02;
static const uint32_t BcTxLockTime = 0x00;

static const uint32_t BcTxCoinbaseVOut = 0xFFFFFFFF;
static const uint32_t BcTxCoinbaseInSequence = 0xFFFFFFFF;

static void bc_block_header_reset(bc_block_header_t *bh) {
    *bh = (bc_block_header_t){
        .version = BcBlockVersion_Default,
        .hash_prev_block = {},
        .hash_merkle_root = {},
        .timestamp = time(NULL),
        .bits = 0,
        .nonce = 0,
    };
}

static void bc_tx_info_reset(bc_tx_info_t *tx) {
    *tx = (bc_tx_info_t){
        .version = BcTxVersion_Basic,
        .lock_time = BcTxLockTime,
    };
}

typedef struct {
    const char *prev_block_hash;
    uint32_t timestamp;
    uint32_t bits;

    const char *obin;
    const char *ohex;
} options_t;

int parse_opts(options_t *opts, int argc, char *argv[]);

int main(int argc, char *argv[]) {
    int ec = 0;
    options_t opts = {0};

    ec = parse_opts(&opts, argc, argv);
    if (ec) {
        return ec;
    }

    bc_block_header_t header = {0};
    bc_tx_info_t tx_info = {0};
    bc_tx_input_array_t tx_i_vec = {.mem = nu_memory_get_default()};
    bc_tx_output_array_t tx_o_vec = {.mem = nu_memory_get_default()};

    bc_block_header_reset(&header);
    bc_tx_info_reset(&tx_info);

    if (opts.prev_block_hash) {
        bc_hash32_from_cspan(&header.hash_prev_block,
                             nu_cspan_from_cstr(opts.prev_block_hash));
    }

    //
    const int UseSegwit = 1;
    const uint8_t OP_HASH160 = 0xa9;
    const uint8_t OP_PUSHBYTES_3 = 0x03;
    const uint8_t OP_PUSHBYTES_20 = 0x14;
    const uint8_t OP_EQUAL = 0x87;

    const uint32_t CurrentHeight = 913802;
    const uint64_t BlockSubsidy = 312500000;
    const uint64_t BlockFees = 0;

    struct {
        uint32_t op_push_3 : 8;
        uint32_t height : 24;
    } ScriptSig = {
        .op_push_3 = OP_PUSHBYTES_3,
        .height = CurrentHeight,
    };
    const size_t ScriptSigSize = sizeof(ScriptSig);

    // struct {
    //     uint8_t op_hash160;
    //     uint8_t op_push_20;
    //     uint8_t script_hash[20];
    //     uint8_t op_equal;
    // } ScriptPubKey = {
    //     .op_hash160 = OP_HASH160,
    //     .op_push_20 = OP_PUSHBYTES_20,
    //     .script_hash = "!ScriptHash!",
    //     .op_equal = OP_EQUAL,
    // };
    struct {
        uint8_t op_true;
    } ScriptPubKey = {1};
    const size_t ScriptPubKeySize = sizeof(ScriptPubKey);

    //

    bc_tx_input_t *const tx_i = nu_dca_consume(&tx_i_vec, 1);
    tx_i[0] = (bc_tx_input_t){
        .txid = {},
        .vout = BcTxCoinbaseVOut,
        .script_sig_size = ScriptSigSize,
        .script_sig = (uint8_t *)&ScriptSig,
        .sequence = BcTxCoinbaseInSequence,
    };

    bc_tx_output_t *const tx_o = nu_dca_consume(&tx_o_vec, 1);
    tx_o[0] = (bc_tx_output_t){
        .amount = BlockSubsidy + BlockFees,
        .script_pub_key_size = ScriptPubKeySize,
        .script_pub_key = (uint8_t *)&ScriptPubKey,
    };

    // Transactions

    tx_info.version = BcTxVersion_Basic;
    if (UseSegwit) {
        tx_info.marker = 0x00;
        tx_info.flags = 0x01;
    }
    tx_info.in_count = nu_dca_size(&tx_i_vec);
    tx_info.in_vec = nu_dca_data(&tx_i_vec);
    tx_info.out_count = nu_dca_size(&tx_o_vec);
    tx_info.out_vec = nu_dca_data(&tx_o_vec);
    tx_info.lock_time = BcTxLockTime;

    // witness (same count as inputs)
    const uint8_t witness_zero_data[32] = {0};
    bc_witness_field_t witness_zero_field = {.size = 32,
                                             .data = witness_zero_data};
    bc_witness_t witness = {0};
    if (UseSegwit) {
        witness.stack_items = 1;
        witness.items = &witness_zero_field;
    }
    tx_info.witness_vec = &witness;

    bc_hash32_t txid = {0};
    bc_tx_txid(&tx_info, &txid);

    // BlockHeader
    header.hash_merkle_root = txid;
    header.timestamp = opts.timestamp ? opts.timestamp : header.timestamp;
    header.bits = opts.bits ? opts.bits : header.bits;
    header.nonce = 0;

    //
    // display result
    //

    bc_fprint_block_header(stdout, &header);
    bc_fprint_tx_info(stdout, &tx_info);

    //
    // write to a buffer
    //

    nu_buffer_t buf = nu_buffer_create();
    nu_dca_reserve(&buf, sizeof(header) + 1024);

    bc_append_block_header(&buf, &header);
    bc_append_varint(&buf, 1); // 1 transaction
    bc_append_transaction(&buf, &tx_info);

    //
    // save to files
    //

    if (opts.obin) {
        FILE *fd = fopen(opts.obin, "wb+");
        if (fd) {
            fwrite(nu_dca_data(&buf), 1, nu_dca_size(&buf), fd);
            fclose(fd);
        }
    }

    if (opts.ohex) {
        const size_t bufsz = nu_dca_size(&buf);
        nu_string_t hex = nu_string_create();
        char *const out = nu_string_consume(&hex, (bufsz * 2) + 1);
        bc_write_hex_forward(out, nu_dca_size(&hex), nu_dca_data(&buf), bufsz);

        FILE *fd = fopen(opts.ohex, "wt+");
        if (fd) {
            fwrite(out, 1, nu_dca_size(&hex), fd);
            fclose(fd);
        }
    }

    nu_dca_release(&buf);
    nu_dca_release(&tx_i_vec);
    nu_dca_release(&tx_o_vec);

    return 0;
}

int parse_opts(options_t *opts, int argc, char *argv[]) {
    int ec = 0;

    for (int i = 1; i < argc; ++i) {
        const char *opt = argv[i];
        const char *val = (i + 1) < argc ? argv[i + 1] : NULL;

        if (strcmp(opt, "-pb") == 0) {
            if (!val) {
                fprintf(stderr, "option '%s' requires value.\n", opt);
                ec = 1;
                break;
            }
            opts->prev_block_hash = val;
            i++;
            continue;
        }

        if (strcmp(opt, "-bits") == 0) {
            if (!val) {
                fprintf(stderr, "option '%s' requires value.\n", opt);
                ec = 1;
                break;
            }
            opts->bits = strtoul(val, NULL, 16);
            i++;
            continue;
        }

        if (strcmp(opt, "-time") == 0) {
            if (!val) {
                fprintf(stderr, "option '%s' requires value.\n", opt);
                ec = 1;
                break;
            }
            opts->timestamp = strtoul(val, NULL, 10);
            i++;
            continue;
        }

        if (strcmp(opt, "-obin") == 0) {
            if (!val) {
                fprintf(stderr, "option '%s' requires value.\n", opt);
                ec = 1;
                break;
            }
            opts->obin = val;
            i++;
            continue;
        }

        if (strcmp(opt, "-ohex") == 0) {
            if (!val) {
                fprintf(stderr, "option '%s' requires value.\n", opt);
                ec = 1;
                break;
            }
            opts->ohex = val;
            i++;
            continue;
        }

        fprintf(stderr, "option '%s' is unknown.\n", opt);
        ec = 1;
        break;
    }

    return ec;
}
