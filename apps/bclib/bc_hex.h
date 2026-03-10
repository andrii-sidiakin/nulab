#ifndef _BC_HEX_H_INCLUDED_
#define _BC_HEX_H_INCLUDED_

#include <nulib/preset.h>

#include <stddef.h>
#include <stdint.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static nu_constexpr uint8_t BcHexCharMap[256] = {
    ['0'] = 0x00, ['1'] = 0x01, ['2'] = 0x02, ['3'] = 0x03, ['4'] = 0x04,
    ['5'] = 0x05, ['6'] = 0x06, ['7'] = 0x07, ['8'] = 0x08, ['9'] = 0x09,
    ['a'] = 0x0A, ['b'] = 0x0B, ['c'] = 0x0C, ['d'] = 0x0D, ['e'] = 0x0E,
    ['f'] = 0x0F, ['A'] = 0X0A, ['B'] = 0X0B, ['C'] = 0X0C, ['D'] = 0X0D,
    ['E'] = 0X0E, ['F'] = 0X0F,
};

static inline uint8_t bc_hex_char_to_byte(const char hex[2]) {
    return (BcHexCharMap[(uint8_t)hex[1]]) |
           (BcHexCharMap[(uint8_t)hex[0]] << 4);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

size_t bc_write_hex_backward(char *buf, size_t bufsz, const void *dat,
                             size_t datsz);

size_t bc_write_hex_forward(char *buf, size_t bufsz, const void *dat,
                            size_t datsz);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
