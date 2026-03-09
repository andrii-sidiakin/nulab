#include "bc_merkle_root.h"
#include "bc_error.h"

#include "sha256.h"

#include <string.h>

static inline void bc_hash32_pair_hash(const bc_hash32_t pair[2],
                                       bc_hash32_t *hash) {
    hash256_digest(pair, BcHashSize * 2, hash->data);
}

int bc_merkle_root(const bc_hash32_array_t *txids, bc_hash32_array_t *buf) {
    size_t itx = nu_dca_size(txids);
    size_t otx = (itx + 1) / 2;

    if (itx < 1) {
        return BcError;
    }

    bc_hash32_t *ip = nu_dca_data(txids);
    bc_hash32_t *op = nu_dca_consume(buf, otx + 2);

    memset(op, 0, sizeof(bc_hash32_t) * (otx + 0));

    const size_t ext = otx;

    while (itx > 1) {
        const int odd = itx % 2;
        otx = (itx + 1) / 2;

        if (odd) {
            op[ext + 0] = ip[itx - 1];
            op[ext + 1] = ip[itx - 1];
        }

        // printf("\nitx: %zu; otx: %zu\n", itx, otx);
        for (size_t i = 0; i < otx - odd; ++i) {
            // printf("[%zu, %zu] -> %zu\n", i * 2, (i * 2) + 1, i);
            bc_hash32_pair_hash(ip + (i * 2), (op + i));
        }

        if (odd) {
            // printf("[%zu, %zu] -> %zu (odd)\n", itx - 1, itx - 1, otx - 1);
            bc_hash32_pair_hash(op + ext, (op + otx - 1));
        }

        ip = op;
        // op = nu_dca_data(buf);

        itx = otx;
    }

    if (itx == 1) {
        op[0] = ip[0];
        // op[ext + 0] = ip[0];
        // op[ext + 1] = ip[0];
        // bc_hash32_pair_hash(op + ext, op);
    }

    return 0;
}
