#ifndef _NULIB_STRING_H_INCLUDED_
#define _NULIB_STRING_H_INCLUDED_

#include "./dca.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

nu_G_define_dca(nu_string_t, char, nu_string, NULIB_API_INLINE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    const char *beg;
    const char *end;
} nu_cspan_t;

#define nu_cspan_make(b, e)                                                    \
    (nu_expect((!(b) && !(e)) || ((b) < (e))),                                 \
     ((nu_cspan_t){.beg = (b), .end = (e)}))

#define nu_cspan_from_ptr_len(b, n)                                            \
    (nu_expect((b) != NULL),                                                   \
     ((nu_cspan_t){.beg = (b), .end = ((b) ? (char *)(b) + (n) : (b))}))

#define nu_cspan_from_string(str)                                              \
    nu_cspan_from_ptr_len(nu_dca_data(str), nu_dca_size(str))

static inline nu_cspan_t nu_cspan_from_cstrn(const char *cs, size_t n) {
    const char *e = cs;
    if (e && n) {
        while (*e && n--) {
            ++e;
        }
    }
    return (nu_cspan_t){cs, e};
}

static inline nu_cspan_t nu_cspan_from_cstr(const char *cs) {
    const char *e = cs;
    if (e) {
        while (*e) {
            ++e;
        }
    }
    return (nu_cspan_t){cs, e};
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
