#include "nulib/memory.h"
#include "nulib/assert.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

NULIB_NoDiscard static inline void *
_nulib_default_consume_impl(void *ctx, void *ptr, size_t nb, size_t a) {
    nu_unused(ctx);
    void *tmp = realloc(ptr, nb);
    nu_assert(tmp != NULL);
    nu_assert((uintptr_t)tmp % a == 0);
    return tmp;
}

static inline void _nulib_default_release_impl(void *ctx, void *ptr, size_t nb,
                                               size_t a) {
    nu_unused(ctx);
    nu_unused(nb);
    nu_unused(a);
    free(ptr);
}

static const nu_memory_ops_t _nulib_default_memory_ops = {
    .consume = _nulib_default_consume_impl,
    .release = _nulib_default_release_impl,
};

static nu_memory_t _nulib_default_memory = {
    .ops = &_nulib_default_memory_ops,
    .ctx = NULL,
};

static nu_memory_t *_nulib_default_memory_ptr = NULL;

nu_memory_t *nu_memory_get_default(void) {
    if (!_nulib_default_memory_ptr) {
        _nulib_default_memory_ptr = &_nulib_default_memory;
    }
    return _nulib_default_memory_ptr;
}

nu_memory_t *nu_memory_set_default(nu_memory_t *m) {
    nu_memory_t *t = nu_memory_get_default();
    _nulib_default_memory_ptr = m ? m : t;
    return t;
}

void *nu_calloc(size_t c, size_t nb) {
    size_t n = 0;
#ifdef NULIB_HAS_CKD_OPS
    if (!nu_expect(!ckd_mul(&n, c, nb))) {
        return NULL;
    }
#else
    n = c * nb;
#endif

    void *ptr = nu_malloc(n);
    if (ptr) {
        memset(ptr, 0, n);
    }
    return ptr;
}
