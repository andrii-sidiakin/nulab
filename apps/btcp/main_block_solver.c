#include <bc_types.h>
#include <bc_utils.h>

#include <sha256.h>

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>

typedef struct {
    const bc_block_header_t *header_ptr;
    const bc_hash32_t *target_ptr;

    uint32_t timestamp;
    uint32_t nonce_min;
    uint32_t nonce_max;

    int id;
    struct timespec T0;
    struct timespec TN;
    uint64_t N;

} worker_ctx_t;

atomic_int CmpMax = 1;
atomic_bool StopScan = false;

#define cmp_load() atomic_load(&CmpMax);

#define cmp_sync(cmp_val)                                                      \
    do {                                                                       \
        int cmp_top = atomic_load(&CmpMax);                                    \
        if (cmp_val > cmp_top) {                                               \
            if (!atomic_compare_exchange_strong(&CmpMax, &cmp_top, cmp_val)) { \
                cmp_val = cmp_top;                                             \
            }                                                                  \
        }                                                                      \
        else {                                                                 \
            cmp_val = cmp_top;                                                 \
        }                                                                      \
    } while (0);

int solve(worker_ctx_t *const task) {
    bc_block_header_t header = *task->header_ptr;
    bc_hash32_t target = *task->target_ptr;

    uint32_t n0 = task->nonce_min;
    uint32_t nN = task->nonce_max;

    header.timestamp = task->timestamp;
    header.nonce = task->nonce_min;

    bc_hash32_t hash_min = {0};
    memset(hash_min.data, 0xFF, 32);

#define SOLVE_V2

#ifdef SOLVE_V2
    sha256_ctx_t ctx_base = {0};
    uint8_t hash_base[Sha256_HashSize] = {0};
    sha256_ctx_t ctx = {0};
#endif
    bc_hash32_t hash = {0};

    char hash_hex[65] = {0};

    int cmp_max = cmp_load();

    uint32_t I = 0;
    uint32_t N = 0;

    timespec_get(&task->T0, TIME_UTC);

#ifdef SOLVE_V2
    sha256_reset(&ctx_base);
    sha256_update(&ctx_base, (const uint8_t *)&header, 76);
#endif

    for (uint32_t n = n0; n <= nN; ++n) {
        N++;
        header.nonce = n;

#ifdef SOLVE_V2
        ctx = ctx_base;
        sha256_update(&ctx, &n, 4);
        sha256_finish(&ctx);
        sha256_result(&ctx, hash_base);
        sha256_reset(&ctx);
        sha256_update(&ctx, hash_base, 32);
        sha256_finish(&ctx);
        sha256_result(&ctx, hash.data);
#else
        hash256_digest(&header, BcBlockHeaderSize, hash.data);
#endif

        int cmp = i256_compare(hash.data, target.data);
        if (cmp > cmp_max) {
            cmp_max = cmp;
            // cmp_sync(cmp_max);
            if (cmp == cmp_max) {
                // hash_min = hash;
                // bc_write_hex_backward(hash_hex, 65, hash.data, 32);
                // printf("%10u:%10u:[%s]\n", header.timestamp, n, hash_hex);
            }
        }
        else if (cmp <= 0) {
            cmp_max = cmp;
            break;
        }

        I++;
        if (!(I % 1'000'000)) {
            // if (atomic_load(&StopScan)) {
            //     break;
            // }
            // cmp_sync(cmp_max);
        }

        if (task->id == 0 && I >= 50'000'000u) {
            // printf("%10u:%10u: %u\n", header.timestamp, n, N);
            I = 0;
        }
    }

    task->N = N;
    timespec_get(&task->TN, TIME_UTC);

    // if (cmp_max <= 0) {
    //     atomic_store(&StopScan, true);
    //     hash_min = hash;
    //     bc_write_hex_backward(hash_hex, 65, hash.data, 32);
    //     printf("%10u:%10u:[%s] - !FOUND!\n", header.timestamp, header.nonce,
    //            hash_hex);
    // }

    return cmp_max;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <block.dat> <target>\n", argv[0]);
        return 1;
    }

    const char *opt_filepath = argv[1];
    const char *opt_target = argv[2];

    if (strlen(opt_target) != 32) {
        printf("error: target is not valid [%s]\n", opt_target);
        return 1;
    }

    bc_block_header_t header = {};
    bc_hash32_t target = {0};

    nu_buffer_t buf = nu_buffer_create();
    btcp_load_file(&buf, opt_filepath);

    const uint8_t *const block_data_ptr = nu_dca_data(&buf);
    const bc_block_header_t *block_header_ptr =
        (const bc_block_header_t *)(block_data_ptr);

    header = *block_header_ptr;
    bc_hash32_from_cspan(&target, nu_cspan_from_cstr(opt_target));

    nu_dca_release(&buf);

    printf("BlockHeader:\n");
    bc_fprint_block_header(stdout, &header);
    bc_fprint_field_hash32(stdout, "target", &target);

    static constexpr int N = 20;
    worker_ctx_t tasks[N] = {0};
    thrd_t ws[N] = {};
    int rs[N] = {0};

    printf("starting %d jobs...\n", N);
    for (int i = 0; i < N; ++i) {
        tasks[i].id = i;
        tasks[i].header_ptr = &header;
        tasks[i].target_ptr = &target;
        tasks[i].timestamp = header.timestamp + i;

        tasks[i].nonce_min = 0;
        // tasks[i].nonce_min = 274148111 - 1000;
        // tasks[i].nonce_min = 662101787 - 1000;

        tasks[i].nonce_max = 0xFFFF'FFFF;
        tasks[i].nonce_max = 50'000'000;

        rs[i] = -1;
        thrd_create(ws + i, (int (*)(void *))solve, &tasks[i]);
    }

    for (int i = 0; i < N; ++i) {
        thrd_join(ws[i], &rs[i]);
    }

    const long double mega_s = 1.e+6;
    const long double nano_s = 1.e-9;
    long double dt_sum = 0;
    uint64_t n_sum = 0;

    printf("finished.\n");

    for (int i = 0; i < N; ++i) {
        const worker_ctx_t *const t = &tasks[i];

        long double dt = (t->TN.tv_sec - t->T0.tv_sec) +
                         (nano_s * (t->TN.tv_nsec - t->T0.tv_nsec));

        long double rate = 1. / dt * t->N / mega_s;

        dt_sum += dt;
        n_sum += t->N;

        printf("job-%02d: %lu in %.02Lfs, rate: %.2LfM, res: %d\n", i, t->N, dt,
               rate, rs[i]);
    }

    printf("Summary:\n");
    long double rate = 1. / dt_sum * n_sum / mega_s * N;
    printf(" complete %lu in %.02Lfs, rate: %.2LfM\n", n_sum, dt_sum / N, rate);

    return 0;
}
