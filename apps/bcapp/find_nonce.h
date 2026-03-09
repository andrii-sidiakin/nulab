#ifndef _FIND_NONCE_H_INCLUDED_
#define _FIND_NONCE_H_INCLUDED_

#include <bc_types.h>

#include <stdint.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int bc_find_nonce(bc_block_header_t *block, const bc_hash32_t *target,
                  uint32_t nonce_min, uint32_t nonce_max);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
