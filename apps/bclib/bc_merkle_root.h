#ifndef _BC_MERKLE_ROOT_H_INCLUDED_
#define _BC_MERKLE_ROOT_H_INCLUDED_

#include "bc_types.h"

int bc_merkle_root(const bc_hash32_array_t *txids, bc_hash32_array_t *buf);

#endif
