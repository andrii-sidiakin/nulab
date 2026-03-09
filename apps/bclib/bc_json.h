#ifndef _BC_JSON_H_INCLUDED_
#define _BC_JSON_H_INCLUDED_

#include <nulib/string.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    nu_cspan_t txid;
    nu_cspan_t hash;
    nu_cspan_t data;
} bc_tx_view_t;

typedef int (*bc_tx_scan_callback_t)(void *ctx, const bc_tx_view_t *tx);

typedef struct {
    bc_tx_scan_callback_t cb;
    void *user_ptr;
} bc_tx_scan_ctx_t;

void tx_view_print(const bc_tx_view_t *tx);

nu_cspan_t json_find_tag_value(nu_cspan_t json, const char *tag);
nu_cspan_t json_find_target(nu_cspan_t json);
nu_cspan_t json_find_prevblock(nu_cspan_t json);
nu_cspan_t json_find_bits(nu_cspan_t json);
nu_cspan_t json_find_mintime(nu_cspan_t json);
nu_cspan_t json_find_height(nu_cspan_t json);

int json_blocktemplate_tx_scan(nu_cspan_t json, bc_tx_scan_ctx_t *ctx);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
