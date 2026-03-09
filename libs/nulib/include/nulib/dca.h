#ifndef _NULIB_DCA_H_INCLUDED_
#define _NULIB_DCA_H_INCLUDED_

#include <nulib/memory.h>
#include <nulib/preset.h>

#include <limits.h>
#include <stdalign.h>
#include <stdckdint.h>
#include <stddef.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    size_t bcap;
    size_t blen;
} nu_dca_base_t;

#define nu_G_dca_mod(T)                                                        \
    struct {                                                                   \
        nu_dca_base_t base;                                                    \
        T elems[];                                                             \
    }

typedef struct {
    nu_dca_base_t **pp;
    size_t data_offset;
    size_t elem_size;
} nu_dca_mbox_t;

#define nu_G_dca(T)                                                            \
    struct {                                                                   \
        nu_memory_t *mem;                                                      \
        nu_G_dca_mod(T) * mod;                                                 \
    }

#define nu_G_mbox_dca(dca)                                                     \
    ((nu_dca_mbox_t){                                                          \
        .pp = (nu_dca_base_t **)&((dca)->mod),                                 \
        .data_offset = offsetof(nu_typeof(*(dca)->mod), elems),                \
        .elem_size = sizeof(*(dca)->mod->elems),                               \
    })

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

NULIB_API
void nu_dca_release_all(nu_dca_base_t **pp, nu_memory_t *mem, size_t off);

NULIB_API
void *nu_dca_reserve_bytes(nu_dca_base_t **pp, nu_memory_t *mem, size_t off,
                           size_t nb);

NULIB_API
void *nu_dca_consume_bytes(nu_dca_base_t **pp, nu_memory_t *mem, size_t off,
                           size_t nb);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

NULIB_API_INLINE
void nu_mbox_dca_release(nu_dca_mbox_t box, nu_memory_t *mem) {
    nu_dca_release_all(box.pp, mem, box.data_offset);
}

NULIB_API_INLINE
void *nu_mbox_dca_reserve(nu_dca_mbox_t box, nu_memory_t *mem, size_t nelems) {
    size_t nbytes = 0;
    if (!nu_expect(!ckd_mul(&nbytes, nelems, box.elem_size))) {
        return NULL;
    }
    return nu_dca_reserve_bytes(box.pp, mem, box.data_offset, nbytes);
}

NULIB_API_INLINE
void *nu_mbox_dca_extend(nu_dca_mbox_t box, nu_memory_t *mem, size_t nelems) {
    if (!nu_expect(box.elem_size > 0)) {
        return NULL;
    }

    const size_t len = (*box.pp) ? (*box.pp)->blen / box.elem_size : 0;

    size_t newlen = 0;
    if (!nu_expect(!ckd_add(&newlen, len, nelems))) {
        return NULL;
    }

    return nu_mbox_dca_reserve(box, mem, newlen);
}

NULIB_API_INLINE
void *nu_mbox_dca_consume(nu_dca_mbox_t box, nu_memory_t *mem, size_t nelems) {
    size_t nbytes = 0;
    if (!nu_expect(!ckd_mul(&nbytes, nelems, box.elem_size))) {
        return NULL;
    }
    return nu_dca_consume_bytes(box.pp, mem, box.data_offset, nbytes);
}

NULIB_API_INLINE
void nu_mbox_dca_clean(nu_dca_mbox_t box, nu_memory_t *mem) {
    if (*box.pp) {
        (void)(mem); // not used for now
        (*box.pp)->blen = 0;
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define nu_dca_elem_size_in_bytes(dca) (nu_G_mbox_dca(dca).elem_size)

#define nu_dca_capacity_in_bytes(dca) ((dca)->mod ? (dca)->mod->base.bcap : 0)

#define nu_dca_size_in_bytes(dca) ((dca)->mod ? (dca)->mod->base.blen : 0)

#define nu_dca_capacity(dca)                                                   \
    (nu_dca_capacity_in_bytes(dca) / nu_dca_elem_size_in_bytes(dca))

#define nu_dca_size(dca)                                                       \
    (nu_dca_size_in_bytes(dca) / nu_dca_elem_size_in_bytes(dca))

#define nu_dca_data(dca)                                                       \
    ((dca)->mod                                                                \
         ? (nu_typeof(*(dca)->mod->elems) *)((char *)((dca)->mod) +            \
                                             nu_G_mbox_dca(dca).data_offset)   \
         : NULL)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define nu_dca_typeof_elem(dca) nu_typeof(nu_dca_data(dca)[0])

#define nu_dca_release(dca) nu_mbox_dca_release(nu_G_mbox_dca(dca), (dca)->mem)

#define nu_dca_reserve(dca, nelems)                                            \
    nu_mbox_dca_reserve(nu_G_mbox_dca(dca), (dca)->mem, nelems)

#define nu_dca_extend(dca, nelems)                                             \
    nu_mbox_dca_extend(nu_G_mbox_dca(dca), (dca)->mem, nelems)

#define nu_dca_consume(dca, nelems)                                            \
    (nu_dca_typeof_elem(dca) *)nu_mbox_dca_consume(nu_G_mbox_dca(dca),         \
                                                   (dca)->mem, nelems)

#define nu_dca_clean(dca) nu_mbox_dca_clean(nu_G_mbox_dca(dca), (dca)->mem)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//
#define nu_G_define_dca_type(dca_t, elem_t) typedef nu_G_dca(elem_t) dca_t

//
#define nu_G_define_dca_base_api_attribs(dca_t, prefix, attribs)               \
                                                                               \
    attribs nu_dca_typeof_elem((dca_t *)(0)) *                                 \
        prefix##_reserve(dca_t *dca, size_t nelems) {                          \
        return nu_dca_reserve(dca, nelems);                                    \
    }                                                                          \
                                                                               \
    attribs nu_dca_typeof_elem((dca_t *)(0)) *                                 \
        prefix##_extend(dca_t *dca, size_t nelems) {                           \
        return nu_dca_extend(dca, nelems);                                     \
    }                                                                          \
                                                                               \
    attribs nu_dca_typeof_elem((dca_t *)(0)) *                                 \
        prefix##_consume(dca_t *dca, size_t nelems) {                          \
        return nu_dca_consume(dca, nelems);                                    \
    }                                                                          \
                                                                               \
    attribs void prefix##_clean(dca_t *dca) {                                  \
        nu_dca_clean(dca);                                                     \
    }                                                                          \
                                                                               \
    attribs const nu_dca_typeof_elem((dca_t *)(0)) *                           \
        prefix##_data(const dca_t *dca) {                                      \
        return nu_dca_data(dca);                                               \
    }                                                                          \
                                                                               \
    attribs size_t prefix##_size(const dca_t *dca) {                           \
        return nu_dca_size(dca);                                               \
    }                                                                          \
                                                                               \
    attribs size_t prefix##_capacity(const dca_t *dca) {                       \
        return nu_dca_capacity(dca);                                           \
    }                                                                          \
                                                                               \
    attribs nu_dca_typeof_elem((dca_t *)(0)) * prefix##_begin(dca_t *dca) {    \
        return nu_dca_data(dca);                                               \
    }                                                                          \
                                                                               \
    attribs nu_dca_typeof_elem((dca_t *)(0)) * prefix##_end(dca_t *dca) {      \
        nu_dca_typeof_elem((dca_t *)(0)) *b = nu_dca_data(dca);                \
        return b ? b + nu_dca_size(dca) : NULL;                                \
    }
// end of nu_G_define_dca_base_api_attribs

//
#define nu_G_define_dca_make_api_attribs(dca_t, prefix, attribs)               \
                                                                               \
    attribs dca_t prefix##_create_with_memory(nu_memory_t *mem) {              \
        return (dca_t){.mem = mem};                                            \
    }                                                                          \
                                                                               \
    attribs dca_t prefix##_create(void) {                                      \
        return prefix##_create_with_memory(nu_memory_get_default());           \
    }                                                                          \
                                                                               \
    attribs void prefix##_release(dca_t *dca) {                                \
        nu_dca_release(dca);                                                   \
    }

//
#define nu_G_define_dca_full_api_attribs(dca_t, prefix, attribs)               \
    nu_G_define_dca_base_api_attribs(dca_t, prefix, attribs)                   \
        nu_G_define_dca_make_api_attribs(dca_t, prefix, attribs)

//
#define nu_G_define_dca_base_api(dca_t, prefix)                                \
    nu_G_define_dca_base_api_attribs(dca_t, prefix, static inline)

//
#define nu_G_define_dca_make_api(dca_t, prefix)                                \
    nu_G_define_dca_make_api_attribs(dca_t, prefix, static inline)

//
#define nu_G_define_dca_full_api(dca_t, prefix)                                \
    nu_G_define_dca_full_api_attribs(dca_t, prefix, static inline)

//
#define nu_G_define_dca(dca_t, elem_t, prefix, attribs)                        \
    nu_G_define_dca_type(dca_t, elem_t);                                       \
    nu_G_define_dca_full_api_attribs(dca_t, prefix, attribs)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
