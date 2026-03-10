#ifndef _SHA256_H_INCLUDED_
#define _SHA256_H_INCLUDED_

#include <nulib/preset.h>

#include <stdint.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

enum {
    Sha256_HashSize = 32,
    Sha256_HashSizeDWord = 8,
    Sha256_BlockSize = 64,
    // Sha256_BlockSizeBits = 512,
};

static nu_constexpr uint32_t Sha256_K[Sha256_BlockSize] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static nu_constexpr uint32_t Sha256_H0[Sha256_HashSizeDWord] = {
    0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A,
    0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19,
};

typedef struct {
    uint32_t hash[Sha256_HashSizeDWord];
    uint8_t block[Sha256_BlockSize];
    uint32_t index;
    uint32_t msglen; // in bytes
} sha256_ctx_t;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int sha256_reset(sha256_ctx_t *ctx);
int sha256_update(sha256_ctx_t *ctx, const void *msg, uint32_t len);
int sha256_finish(sha256_ctx_t *ctx);
int sha256_result(const sha256_ctx_t *ctx, uint8_t buf[Sha256_HashSize]);

int sha256_begin(sha256_ctx_t *ctx, const uint8_t *msg, uint32_t len);
int sha256_cont_then_sha256_result(sha256_ctx_t *ctx, const uint8_t *msg,
                                   uint32_t len,
                                   uint8_t mid_sum[Sha256_BlockSize]);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static inline int sha256_digest(const void *msg, uint32_t len,
                                uint8_t buf[Sha256_HashSize]) {
    sha256_ctx_t ctx = {0};
    int ec = 0;
    ec = ec ? ec : sha256_reset(&ctx);
    ec = ec ? ec : sha256_update(&ctx, msg, len);
    ec = ec ? ec : sha256_finish(&ctx);
    ec = ec ? ec : sha256_result(&ctx, buf);
    return ec;
}

static inline int hash256_digest(const void *msg, uint32_t len,
                                 uint8_t buf[Sha256_HashSize]) {
    uint8_t tmp[Sha256_HashSize];
    int ec = 0;
    ec = ec ? ec : sha256_digest(msg, len, tmp);
    ec = ec ? ec : sha256_digest(tmp, Sha256_HashSize, buf);
    return ec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
