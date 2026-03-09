#include "bc_hex.h"

static constexpr char HexChars[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

size_t bc_write_hex_backward(char *buf, size_t bufsz, const void *dat,
                             size_t datsz) {
    if (bufsz < (datsz * 2) + 1) {
        return 0;
    }
    if (datsz < 1) {
        buf[0] = '\0';
        return 0;
    }

    uint32_t i = 0;
    uint32_t j = datsz - 1;

    const uint8_t *const bytes = dat;

    uint8_t hi = 0;
    uint8_t lo = 0;
    for (; i < datsz; ++i, --j) {
        lo = (bytes[j] & 0xF0) >> 4;
        hi = (bytes[j] & 0x0F);
        buf[(i * 2) + 0] = *(HexChars + lo);
        buf[(i * 2) + 1] = *(HexChars + hi);
    }

    buf[i * 2] = 0;

    return i * 2;
}

size_t bc_write_hex_forward(char *buf, size_t bufsz, const void *dat,
                            size_t datsz) {
    if (bufsz < (datsz * 2) + 1) {
        return 0;
    }
    if (datsz < 1) {
        buf[0] = '\0';
        return 0;
    }

    uint32_t i = 0;
    uint32_t j = 0;

    const uint8_t *const bytes = dat;

    uint8_t hi = 0;
    uint8_t lo = 0;
    for (; i < datsz; ++i, ++j) {
        lo = (bytes[j] & 0xF0) >> 4;
        hi = (bytes[j] & 0x0F);
        buf[(i * 2) + 0] = *(HexChars + lo);
        buf[(i * 2) + 1] = *(HexChars + hi);
    }

    buf[i * 2] = '\0';

    return i * 2;
}
