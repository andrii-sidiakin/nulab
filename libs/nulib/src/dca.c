#include "nulib/dca.h"
#include "nulib/assert.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void nu_dca_release_all(nu_dca_base_t **pp, nu_memory_t *mem, size_t off) {
    nu_assert(pp != NULL);
    nu_assert(off >= sizeof(nu_dca_base_t));

    nu_dca_base_t *p = *pp;
    if (p) {
        nu_memory_release_aligned(mem, p, off + p->bcap,
                                  nu_alignof(nu_dca_base_t));
        *pp = NULL;
    }
}

void *nu_dca_reserve_bytes(nu_dca_base_t **pp, nu_memory_t *mem, size_t off,
                           size_t nb) {
    nu_assert(pp != NULL);
    nu_assert(off >= sizeof(nu_dca_base_t));

    const size_t size = off + nb;

    nu_dca_base_t *p = *pp;
    if (p == NULL) {
        p = nu_memory_consume_aligned(mem, size, nu_alignof(nu_dca_base_t));
        if (p) {
            *p = (nu_dca_base_t){0};
        }
    }
    else if (p->bcap < nb) {
        p = nu_memory_reconsume_aligned(mem, *pp, size,
                                        nu_alignof(nu_dca_base_t));
    }
    else {
        return (char *)p + off;
    }

    if (!nu_ensure(p != NULL)) {
        return NULL;
    }

    p->bcap = nb;
    p->blen = p->blen > nb ? nb : p->blen;

    *pp = p;
    return (char *)p + off;
}

void *nu_dca_consume_bytes(nu_dca_base_t **pp, nu_memory_t *mem, size_t off,
                           size_t nb) {
    nu_assert(pp != NULL);
    nu_assert(off >= sizeof(nu_dca_base_t));

    const size_t cap = *pp ? (*pp)->bcap : 0;
    const size_t len = *pp ? (*pp)->blen : 0;
    const size_t req_cap = len + nb;

    if (!nu_ensure(req_cap >= len)) {
        return NULL;
    }

    // nu_assert(req_cap > 0 && "zero-size allocation?");

    void *data = NULL;
    if (!*pp || cap < req_cap) {
        data = nu_dca_reserve_bytes(pp, mem, off, req_cap);
    }
    else {
        data = (char *)(*pp) + off;
    }

    if (!nu_ensure(data != NULL)) {
        return NULL;
    }

    (*pp)->blen = len + nb;

    return (char *)data + len;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
