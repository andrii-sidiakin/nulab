#include "sha256.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define SHA256_ALWAYS_INLINE static inline __attribute__((always_inline))

SHA256_ALWAYS_INLINE uint32_t shl(uint32_t x, uint32_t n) {
    return x << (31 & n);
}

SHA256_ALWAYS_INLINE uint32_t shr(uint32_t x, uint32_t n) {
    return x >> (31 & n);
}

[[maybe_unused]] SHA256_ALWAYS_INLINE uint32_t rtl(uint32_t x, uint32_t n) {
    return shl(x, n) | shr(x, 32 - n);
}

SHA256_ALWAYS_INLINE uint32_t rtr(uint32_t x, uint32_t n) {
    return shr(x, n) | shl(x, 32 - n);
}

SHA256_ALWAYS_INLINE uint32_t bsig0(uint32_t x) {
    return rtr(x, 2) ^ rtr(x, 13) ^ rtr(x, 22);
}

SHA256_ALWAYS_INLINE uint32_t bsig1(uint32_t x) {
    return rtr(x, 6) ^ rtr(x, 11) ^ rtr(x, 25);
}

SHA256_ALWAYS_INLINE uint32_t ssig0(uint32_t x) {
    return rtr(x, 7) ^ rtr(x, 18) ^ shr(x, 3);
}

SHA256_ALWAYS_INLINE uint32_t ssig1(uint32_t x) {
    return rtr(x, 17) ^ rtr(x, 19) ^ shr(x, 10);
}

SHA256_ALWAYS_INLINE uint32_t sha_ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

SHA256_ALWAYS_INLINE uint32_t sha_maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static inline void sha256_solve_block(const uint8_t block[Sha256_BlockSize],
                                      uint32_t hash[Sha256_HashSizeDWord]) {
    alignas(64) uint32_t w[Sha256_BlockSize] = {0};

    const uint8_t *msg = block;
    for (unsigned t = 0, q = 0; t < 16; t += 1, q += 4) {
        w[t] = (msg[q + 0] << 24) | //
               (msg[q + 1] << 16) | //
               (msg[q + 2] << 8) |  //
               (msg[q + 3]);        //
    }

    for (unsigned t = 16; t < 64; ++t) {
        w[t] = ssig1(w[t - 2]) + w[t - 7] + ssig0(w[t - 15]) + w[t - 16];
    }

    uint32_t a = hash[0];
    uint32_t b = hash[1];
    uint32_t c = hash[2];
    uint32_t d = hash[3];
    uint32_t e = hash[4];
    uint32_t f = hash[5];
    uint32_t g = hash[6];
    uint32_t h = hash[7];
    uint32_t t1;
    uint32_t t2;

    for (unsigned t = 0; t < Sha256_BlockSize; ++t) {
        t1 = h + bsig1(e) + sha_ch(e, f, g) + Sha256_K[t] + w[t];
        t2 = bsig0(a) + sha_maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    hash[0] += a;
    hash[1] += b;
    hash[2] += c;
    hash[3] += d;
    hash[4] += e;
    hash[5] += f;
    hash[6] += g;
    hash[7] += h;
}

SHA256_ALWAYS_INLINE int sha256_submit(sha256_ctx_t *ctx) {
    sha256_solve_block(ctx->block, ctx->hash);
    ctx->index = 0;
    return 0;
}

SHA256_ALWAYS_INLINE void sha256_fill_zeros(sha256_ctx_t *ctx) {
    uint32_t n = Sha256_BlockSize - ctx->index;
    memset(ctx->block + ctx->index, 0, n);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int sha256_reset(sha256_ctx_t *ctx) {
#if 0
    for (unsigned i = 0; i < Sha256_BlockSize; ++i) {
        ctx->block[i] = 0;
    }
    for (unsigned i = 0; i < Sha256_HashSizeDWord; ++i) {
        ctx->hash[i] = Sha256_H0[i];
    }

    ctx->msglen = 0;
    ctx->index = 0;
#else
    memset(ctx, 0, sizeof(*ctx));
    memcpy(ctx->hash, Sha256_H0, sizeof(Sha256_H0));
    // for (unsigned i = 0; i < Sha256_HashSizeDWord; ++i) {
    //     ctx->hash[i] = Sha256_H0[i];
    // }
#endif

    return 0;
}

int sha256_update(sha256_ctx_t *ctx, const void *msg, uint32_t len) {
    const uint8_t *ptr = msg;
    const uint32_t msglen = ctx->msglen + len;

    if (len == 0) {
        return 0;
    }
    if (msglen < ctx->msglen) {
        return EOVERFLOW;
    }

    ctx->msglen = msglen;

#if 0
    assert(ctx->index < Sha256_BlockSize);

    const size_t pad = Sha256_BlockSize - ctx->index;
    const size_t num = pad < len ? pad : len;

    if (num) {
        memcpy(ctx->block + ctx->index, ptr, num);
        len -= num;
        ptr += num;
    }

    if (num != pad) {
        assert(len == 0);
        ctx->index += num;
    }
    else {
        sha256_submit(ctx);

        assert(ctx->index == 0);
        for (; len >= Sha256_BlockSize; len -= Sha256_BlockSize) {
            memcpy(ctx->block, ptr, Sha256_BlockSize);
            ptr += Sha256_BlockSize;
            sha256_submit(ctx);
        }
    }

#else

    for (uint32_t i = 0; i < len; ++i) {
        ctx->block[ctx->index++] = *ptr++;
        if (ctx->index == Sha256_BlockSize) {
            sha256_submit(ctx);
            ctx->index = 0;
        }
    }

#endif

    return 0;
}

int sha256_finish(sha256_ctx_t *ctx) {
    constexpr uint8_t pad_byte = 0x80;
    constexpr uint32_t size_off = 56;

    const uint64_t bitlen = 8ULL * ctx->msglen;

    if (ctx->index >= size_off) {
        ctx->block[ctx->index++] = pad_byte;
        sha256_fill_zeros(ctx);
        sha256_submit(ctx);
        assert(ctx->index == 0);
    }
    else {
        ctx->block[ctx->index++] = pad_byte;
    }

    sha256_fill_zeros(ctx);

#if 0
    for (unsigned i = 0; i < 8; ++i) {
        ctx->block[63 - i] = (uint8_t)((bitlen >> (8 * i)) & 0xFF);
    }
#else
    ctx->block[56] = (uint8_t)(bitlen >> 56);
    ctx->block[57] = (uint8_t)(bitlen >> 48);
    ctx->block[58] = (uint8_t)(bitlen >> 40);
    ctx->block[59] = (uint8_t)(bitlen >> 32);
    ctx->block[60] = (uint8_t)(bitlen >> 24);
    ctx->block[61] = (uint8_t)(bitlen >> 16);
    ctx->block[62] = (uint8_t)(bitlen >> 8);
    ctx->block[63] = (uint8_t)(bitlen);
    ctx->index = 64;
#endif

    return sha256_submit(ctx);
}

int sha256_result(const sha256_ctx_t *ctx, uint8_t buf[Sha256_HashSize]) {
#if 1
    uint8_t *dst = buf;
    for (int i = 0; i < 8; ++i) {
        for (int j = 3; j >= 0; --j) {
            *dst++ = (ctx->hash[i] >> j * 8) & 0xFF;
        }
    }
#elif 1
    for (unsigned i = 0, j = 0; i < Sha256_HashSize; i += 4, j += 1) {
        uint32_t h = *(ctx->hash + j);
        buf[i + 0] = (uint8_t)(h >> 24);
        buf[i + 1] = (uint8_t)(h >> 16);
        buf[i + 2] = (uint8_t)(h >> 8);
        buf[i + 3] = (uint8_t)(h >> 0);
    }
#else
    for (unsigned i = 0; i < Sha256_HashSize; ++i) {
        buf[i] = ctx->hash[i >> 2] >> (8 * (3 - (i & 0x03)));
    }
#endif
    return 0;
}

int sha256_begin(sha256_ctx_t *ctx, const uint8_t *msg, uint32_t len) {
    int ec = sha256_reset(ctx);
    if (!ec) {
        for (uint32_t i = 0; i < len; ++i) {
            ctx->block[ctx->index++] = msg[i];
            if (ctx->index == Sha256_BlockSize) {
                sha256_solve_block(ctx->block, ctx->hash);
                ctx->index = 0;
            }
        }
        ctx->msglen = len;
    }
    return ec;
}

int sha256_cont_then_sha256_result(sha256_ctx_t *ctx, const uint8_t *msg,
                                   uint32_t len,
                                   uint8_t mid_sum[Sha256_BlockSize]) {
    for (uint32_t i = 0; i < len; ++i) {
        ctx->block[ctx->index++] = msg[i];
        if (ctx->index == Sha256_BlockSize) {
            sha256_solve_block(ctx->block, ctx->hash);
            ctx->index = 0;
        }
    }
    ctx->msglen += len;

    int ec = sha256_finish(ctx);
    if (ec) {
        return ec;
    }

    // uint8_t sum1[Sha256_BlockSize];
    sha256_result(ctx, mid_sum);

    sha256_reset(ctx);
    sha256_update(ctx, mid_sum, Sha256_BlockSize);
    ec = sha256_finish(ctx);

    return ec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
