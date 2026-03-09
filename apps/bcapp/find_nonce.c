#include "find_nonce.h"

#include <bc_error.h>
#include <bc_utils.h>
#include <sha256.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <threads.h>
#include <time.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifndef NDEBUG
#define TRACK_LEAST_NONCE
#else
#define TRACK_LEAST_NONCE
#endif

bool FindNonceStopFlag = false;
// atomic_bool FindNonceCanceled = false;

[[maybe_unused]]
static int find_nonce_v1(nu_bseq_t buf, uint32_t nonce_min, uint32_t nonce_max,
                         const uint8_t target[Sha256_HashSize]) {
    if (buf.size < BcBlockHeaderSize) {
        return BcInvalidBlockHeader;
    }

    int ec = 0;

    uint8_t expect[Sha256_HashSize];
    memcpy(expect, target, Sha256_HashSize);

    uint8_t hash_base[Sha256_HashSize] = {0};
    uint8_t hash[Sha256_HashSize] = {0};

    sha256_ctx_t ctx = {0};
    sha256_ctx_t ctx_base = {0};

    const int noff = 80 - 4;
    uint32_t *const nonce_ptr = (uint32_t *)(buf.data + noff);

    uint32_t nonce = nonce_min;

#ifdef TRACK_LEAST_NONCE
    uint32_t nonce_best = nonce_min;
    int cmp_max = INT_MIN;
#endif

    sha256_reset(&ctx_base);
    sha256_update(&ctx_base, buf.data, noff);

    goto proc;

next_nonce:
    if (nonce >= nonce_max) {
#ifdef TRACK_LEAST_NONCE
        *nonce_ptr = nonce_best;
        bc_hash32_t hash = {0};
        bc_hash32_hex_t hash_str = {0};
        hash256_digest(buf.data, buf.size, hash.data);
        bc_hash32_str_make(&hash_str, &hash);
        printf("... (%2i) hash32 [%s] %u\n", cmp_max - 1, hash_str.data,
               nonce_best);
#endif
        return BcNotFound;
    }
    if (FindNonceStopFlag) {
        return BcCanceled;
    }

    nonce++;

proc:
    *nonce_ptr = nonce;

    ctx = ctx_base;
    ec = ec ? ec : sha256_update(&ctx, buf.data + noff, buf.size - noff);
    ec = ec ? ec : sha256_finish(&ctx);
    ec = ec ? ec : sha256_result(&ctx, hash_base);

    ec = ec ? ec : sha256_digest(hash_base, Sha256_HashSize, hash);

    if (ec) {
        return ec;
    }

    int cmp = i256_compare(hash, expect);
#ifdef TRACK_LEAST_NONCE
    if (cmp > cmp_max) {
        cmp_max = cmp;
        nonce_best = nonce;
    }
#endif
    if (cmp) {
        goto next_nonce;
    }

    FindNonceStopFlag = true;
    return BcSuccess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

[[maybe_unused]]
static int find_nonce_v2(nu_bseq_t buf, uint32_t nonce_min, uint32_t nonce_max,
                         const uint8_t target[Sha256_HashSize]) {
    if (buf.size < BcBlockHeaderSize) {
        return BcInvalidBlockHeader;
    }

    int ec = 0;

    uint8_t expect[Sha256_HashSize];
    memcpy(expect, target, Sha256_HashSize);

    uint8_t hash_base[Sha256_HashSize] = {0};
    uint8_t hash[Sha256_HashSize] = {0};

    sha256_ctx_t ctx = {0};
    sha256_ctx_t ctx_base = {0};

    const int noff = 80 - 4;
    uint32_t *const nonce_ptr = (uint32_t *)(buf.data + noff);

    uint32_t nonce = nonce_min;

#ifdef TRACK_LEAST_NONCE
    uint32_t nonce_best = nonce_min;
    int cmp_max = INT_MIN;
#endif

    sha256_begin(&ctx_base, buf.data, noff);

    goto proc;

next_nonce:
    if (nonce >= nonce_max) {
#ifdef TRACK_LEAST_NONCE
        *nonce_ptr = nonce_best;
        printf("... lowest with nonce: %12u (cmp=%i)\n", nonce_best, cmp_max);
#endif
        return BcNotFound;
    }
    if (FindNonceStopFlag) {
        return BcCanceled;
    }

    nonce++;

proc:
    *nonce_ptr = nonce;

    ctx = ctx_base;
    ec = ec ? ec
            : sha256_cont_then_sha256_result(&ctx, buf.data + noff,
                                             buf.size - noff, hash_base);
    ec = ec ? ec : sha256_result(&ctx, hash);

    if (ec) {
        return ec;
    }

    int cmp = i256_compare(hash, expect);
#ifdef TRACK_LEAST_NONCE
    if (cmp > cmp_max) {
        cmp_max = cmp;
        nonce_best = nonce;
    }
#endif
    if (cmp) {
        goto next_nonce;
    }

    FindNonceStopFlag = true;
    return BcSuccess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    nu_bseq_t buf;
    uint32_t min;
    uint32_t max;
    const uint8_t *target;
} find_nonce_worker_job_t;

[[maybe_unused]]
static int find_nonce_worker_v1(void *data) {
    const find_nonce_worker_job_t *d = data;
    return find_nonce_v1(d->buf, d->min, d->max, d->target);
}

[[maybe_unused]]
static int find_nonce_worker_v2(void *data) {
    const find_nonce_worker_job_t *d = data;
    return find_nonce_v2(d->buf, d->min, d->max, d->target);
}

[[maybe_unused]]
static int find_nonce_par(nu_bseq_t buf, uint32_t nonce_min, uint32_t nonce_max,
                          const uint8_t target[Sha256_HashSize],
                          int (*worker)(void *)) {
    constexpr int N = 20;
    find_nonce_worker_job_t jobs[N] = {0};
    thrd_t ws[N] = {0};
    int rs[N] = {0};

    const uint32_t nd = ((uint64_t)nonce_max - nonce_min + 1) / N;
    nu_assert(nd > 0);

    uint32_t n = nonce_min;
    for (int i = 0; i < N; ++i) {
        find_nonce_worker_job_t *const jp = jobs + i;

        jp->buf.size = buf.size;
        jp->buf.data = malloc(buf.size);
        memcpy(jp->buf.data, buf.data, buf.size);

        jp->min = n;
        jp->max = n + (nd - 1);
        jp->target = target;

        n += nd;
    }
    jobs[N - 1].max = nonce_max;

    for (int i = 0; i < N; ++i) {
        // printf("start : %2i - %u..%u (%u)\n", i, jobs[i].min, jobs[i].max,
        //        (jobs[i].max - jobs[i].min));
        thrd_create(ws + i, worker, jobs + i);
    }

    for (int i = 0; i < N; ++i) {
        thrd_join(ws[i], rs + i);
    }

    int ec = BcNotFound;
    for (int i = 0; i < N; ++i) {
        // printf("finish: %2i - res:%i\n", i, rs[i]);
        if (ec && rs[i] == 0) {
            memcpy(buf.data, jobs[i].buf.data, buf.size);
            ec = 0;
        }
        free(jobs[i].buf.data);
    }

    return ec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int bc_find_nonce(bc_block_header_t *block, const bc_hash32_t *target,
                  const uint32_t nonce_min, const uint32_t nonce_max) {

    const uint32_t nonce_count = nonce_max - nonce_min;

    nu_bseq_t buf = nu_bseq_make(block, sizeof(*block));

    struct timespec rt1 = {0};
    struct timespec rt2 = {0};
    clock_t ct1 = 0;
    clock_t ct2 = 0;

    printf("looking for nonce in range=[%u..%u]...\n", nonce_min, nonce_max);
    timespec_get(&rt1, TIME_UTC);
    ct1 = clock();

#if 1
    printf("find_nonce: v1\n");
    int ec = find_nonce_v1(buf, nonce_min, nonce_max, target->data);
#elif 0
    printf("find_nonce: v2\n");
    int ec = find_nonce_v2(buf, nonce_min, nonce_max, target->data);
#elif 1
    printf("find_nonce: v1-par\n");
    int ec = find_nonce_par(buf, nonce_min, nonce_max, target->data,
                            find_nonce_worker_v1);
#elif 1
    printf("find_nonce: v2-par\n");
    int ec = find_nonce_par(buf, nonce_min, nonce_max, target->data,
                            find_nonce_worker_v2);
#else
#error "choose one"
#endif

    timespec_get(&rt2, TIME_UTC);
    ct2 = clock();

    printf("... done ec=%d, stats:\n", ec);

    const long double ctd = ((long double)(ct2 - ct1)) / CLOCKS_PER_SEC;
    const long double rtd = (rt2.tv_sec - rt1.tv_sec) +
                            ((rt2.tv_nsec - rt1.tv_nsec) / 1000000000.0);
    const long double rate_s = 1.e-6;

    printf("  proc: %u step in cpu=%.03Lf, real=%.03Lf\n", nonce_count, ctd,
           rtd);
    printf("  rate: cpu=%.3LfM, real=%.3LfM\n",
           (1. / ctd * nonce_count) * rate_s,
           (1. / rtd * nonce_count) * rate_s);

    return ec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
