#ifndef _NULIB_MEMORY_H_INCLUDED_
#define _NULIB_MEMORY_H_INCLUDED_

#include "./assert.h"
#include "./preset.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum {
    NuAlignNone = 0,
#ifdef max_align_t
    NuAlignMax = nu_alignof(max_align_t),
#else
    NuAlignMax = nu_alignof(long double), // TODO
#endif
};

typedef void *(*nu_memory_consume_fn_t)(void *ctx, void *ptr, size_t nb,
                                        size_t a);

typedef void (*nu_memory_release_fn_t)(void *ctx, void *ptr, size_t nb,
                                       size_t a);

typedef struct {
    nu_memory_consume_fn_t consume;
    nu_memory_release_fn_t release;
} nu_memory_ops_t;

typedef struct {
    const nu_memory_ops_t *ops;
    void *ctx;
} nu_memory_t;

NULIB_API nu_memory_t *nu_memory_get_default(void);
NULIB_API nu_memory_t *nu_memory_set_default(nu_memory_t *m);

NULIB_NoDiscard NULIB_API_INLINE void *
nu_memory_consume_aligned(nu_memory_t *m, size_t nb, size_t a) {
    nu_assert(m != NULL);
    nu_assert(m->ops != NULL);
    nu_assert(m->ops->consume != NULL);
    return m->ops->consume(m->ctx, NULL, nb, a);
}

NULIB_NoDiscard NULIB_API_INLINE void *
nu_memory_reconsume_aligned(nu_memory_t *m, void *ptr, size_t nb, size_t a) {
    nu_assert(m != NULL);
    nu_assert(m->ops != NULL);
    nu_assert(m->ops->consume != NULL);
    return m->ops->consume(m->ctx, ptr, nb, a);
}

NULIB_API_INLINE void nu_memory_release_aligned(nu_memory_t *m, void *ptr,
                                                size_t nb, size_t a) {
    nu_assert(m != NULL);
    nu_assert(m->ops != NULL);
    nu_assert(m->ops->release != NULL);
    m->ops->release(m->ctx, ptr, nb, a);
}

NULIB_API_INLINE void *nu_memory_consume(nu_memory_t *m, size_t nb) {
    return nu_memory_consume_aligned(m, nb, NuAlignMax);
}

NULIB_API_INLINE void *nu_memory_reconsume(nu_memory_t *m, void *ptr,
                                           size_t nb) {
    return nu_memory_reconsume_aligned(m, ptr, nb, NuAlignMax);
}

NULIB_API_INLINE void nu_memory_release(nu_memory_t *m, void *ptr, size_t nb) {
    nu_memory_release_aligned(m, ptr, nb, NuAlignNone);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

NULIB_API_INLINE void nu_free(void *ptr) {
    nu_memory_release(nu_memory_get_default(), ptr, 0);
}

NULIB_API_INLINE void *nu_malloc(size_t nb) {
    return nu_memory_consume(nu_memory_get_default(), nb);
}

NULIB_API_INLINE void *nu_realloc(void *ptr, size_t nb) {
    return nu_memory_reconsume(nu_memory_get_default(), ptr, nb);
}

NULIB_API void *nu_calloc(size_t c, size_t nb);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
