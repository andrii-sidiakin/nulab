#include "bc_json.h"
#include "bc_types.h"

#include <string.h>

static const char Tag_txns[] = "\"transactions\"";
static const char Tag_data[] = "\"data\"";
static const char Tag_txid[] = "\"txid\"";
static const char Tag_hash[] = "\"hash\"";
static const char Tag_target[] = "\"target\"";
static const char Tag_previousblockhash[] = "\"previousblockhash\"";
static const char Tag_bits[] = "\"bits\"";
static const char Tag_mintime[] = "\"mintime\"";
static const char Tag_height[] = "\"height\"";

// enum {
//     BcJson_Target,
//     BcJson_PrevBlockHash,
//     BcJson_Bits,
//     BcJson_MinTime,
//
//     BcJson_KnownTagsCount,
// };
// static const char *JsonTag[BcJson_KnownTagsCount] = {
//     [BcJson_Target] = "\"target\"",
//     [BcJson_PrevBlockHash] = "\"previousblockhash\"",
//     [BcJson_Bits] = "\"bits\"",
//     [BcJson_MinTime] = "\"mintime\"",
// };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum {
    PrintTagOffset = 8,
};

void tx_view_print(const bc_tx_view_t *tx) {
    printf("TX:\n");

    printf(".%*s: [%.*s...], %ld bytes\n", PrintTagOffset, "data",
           BcTxDataStringSizeMin - 3, tx->data.beg,
           tx->data.end - tx->data.beg);

    printf(".%*s: [%.*s]\n", PrintTagOffset, "txid", BcHashStringSize,
           tx->txid.beg);

    printf(".%*s: [%.*s]\n", PrintTagOffset, "hash", BcHashStringSize,
           tx->hash.beg);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

nu_cspan_t json_find_tag_value(nu_cspan_t json, const char *tag) {
    const size_t tag_len = strlen(tag);

    const char *tmp = strstr(json.beg, tag);
    if (!tmp) {
        return (nu_cspan_t){};
    }
    tmp += tag_len;

    const char *val_beg = tmp + strspn(tmp, "\": \n\t");
    const char *val_end = val_beg + strcspn(val_beg, "\",");

    if (json.end < val_end) {
        return (nu_cspan_t){};
    }

    return (nu_cspan_t){.beg = val_beg, .end = val_end};
}

nu_cspan_t json_find_target(nu_cspan_t json) {
    return json_find_tag_value(json, Tag_target);
}

nu_cspan_t json_find_prevblock(nu_cspan_t json) {
    return json_find_tag_value(json, Tag_previousblockhash);
}

nu_cspan_t json_find_bits(nu_cspan_t json) {
    return json_find_tag_value(json, Tag_bits);
}

nu_cspan_t json_find_mintime(nu_cspan_t json) {
    return json_find_tag_value(json, Tag_mintime);
}

nu_cspan_t json_find_height(nu_cspan_t json) {
    return json_find_tag_value(json, Tag_height);
}

int json_blocktemplate_tx_scan(nu_cspan_t json, bc_tx_scan_ctx_t *ctx) {
    const char *cur = json.beg;
    const char *end = json.end;
    const char *tmp = NULL;

    bc_tx_view_t tx = {0};

    tmp = strstr(cur, Tag_txns);
    if (tmp == NULL) {
        fprintf(stderr, "tag [%s] not found\n", Tag_txns);
        return -1;
    }
    tmp = tmp + sizeof(Tag_txns);

    while (tmp < end) {
        cur = tmp;

        const nu_cspan_t span = {.beg = cur, .end = end};

        nu_cspan_t data = json_find_tag_value(span, Tag_data);
        if (!data.beg) {
            fprintf(stderr, "tag [%s] not found\n", Tag_data);
            break;
        }
        tmp = tmp < data.end ? data.end : tmp;

        nu_cspan_t txid = json_find_tag_value(span, Tag_txid);
        if (!txid.beg) {
            fprintf(stderr, "tag [%s] not found\n", Tag_txid);
            break;
        }
        tmp = tmp < txid.end ? txid.end : tmp;

        nu_cspan_t hash = json_find_tag_value(span, Tag_hash);
        if (!hash.beg) {
            fprintf(stderr, "tag [%s] not found\n", Tag_hash);
            break;
        }
        tmp = tmp < hash.end ? hash.end : tmp;

        tx = (bc_tx_view_t){.txid = txid, .hash = hash, .data = data};
        if (ctx->cb(ctx->user_ptr, &tx)) {
            break;
        }
    }

    return 0;
}
