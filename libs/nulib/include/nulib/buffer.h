#ifndef _NULIB_BUFFER_H_INCLUDED_
#define _NULIB_BUFFER_H_INCLUDED_

#include "./dca.h"

#include <stdint.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

nu_G_define_dca(nu_buffer_t, uint8_t, nu_buffer, NULIB_API_INLINE);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    uint8_t *data;
    size_t size;
} nu_bseq_t;

#define nu_bseq_make(p, n) ((nu_bseq_t){.data = (uint8_t *)(p), .size = (n)})

#define nu_bseq_make_empty() nu_bseq_make(NULL, 0)

#define nu_bseq_from_dca(dca)                                                  \
    nu_bseq_make(nu_dca_data(dca), nu_dca_size_in_bytes(dca))

#define nu_bseq_begin(bs) (bs.data)

#define nu_bseq_end(bs) (bs.data ? bs.data + bs.size : bs.data)

#define nu_bseq_is_empty(bs) (!bs.data || !bs.size)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
