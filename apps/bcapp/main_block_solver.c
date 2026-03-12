#include <bc_types.h>
#include <bc_utils.h>

#include <sha256.h>

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>

typedef struct {
    bc_block_header_t header;
    bc_hash32_t target;
    uint32_t nonce_min;
    uint32_t nonce_max;
} find_nonce_task_t;

typedef struct {
    bc_block_header_t header;
    bc_hash32_t target;

    uint32_t timestamp;
    uint32_t timestamp_max;
    uint32_t nonce;
    uint32_t nonce_chunks;

} find_nonce_task_pool_t;

static int find_nonce(find_nonce_task_t *task) {
    sha256_ctx_t ctx = {0};
    sha256_ctx_t tmp_ctx = {0};
    uint8_t tmp_hash[BcHashSize] = {0};
    uint8_t hash[BcHashSize] = {0};

    sha256_reset(&tmp_ctx);
    sha256_update(&tmp_ctx, &task->header, 76);

    for (uint64_t n = task->nonce_min; n <= task->nonce_max; ++n) {
        task->header.nonce = n;

        ctx = tmp_ctx;
        sha256_update(&ctx, &task->header.nonce, 4);
        sha256_finish(&ctx);
        sha256_result(&ctx, tmp_hash);
        sha256_reset(&ctx);
        sha256_update(&ctx, tmp_hash, BcHashSize);
        sha256_finish(&ctx);
        sha256_result(&ctx, hash);

        if (i256_le(hash, task->target.data)) {
            return 1;
        }
    }

    return 0;
}

static int find_nonce_next_task(find_nonce_task_pool_t *pool,
                                find_nonce_task_t *task) {
    const uint32_t nonce_min = 0;
    const uint32_t nonce_max = 0xFFFF'FFFF;
    const uint32_t nonce_chunk = (nonce_max - nonce_min) / pool->nonce_chunks;

    uint64_t a = pool->nonce;
    uint64_t b = (a + nonce_chunk > nonce_max) ? nonce_max : a + nonce_chunk;
    uint32_t t = pool->timestamp;

    if (t > pool->timestamp_max) {
        return 0;
    }

    if (b == nonce_max) {
        pool->timestamp += 1;
        pool->nonce = nonce_min;
    }
    else {
        pool->nonce = b + 1;
    }

    task->nonce_min = a;
    task->nonce_max = b;

    task->target = pool->target;
    task->header = pool->header;
    task->header.timestamp = t;
    task->header.nonce = a;

    return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// void find_nonce(const bc_block_header_t *header, const bc_hash32_t *target) {
//     static constexpr int N = 20;
//     worker_ctx_t tasks[N] = {0};
//     thrd_t ws[N] = {};
//     int rs[N] = {0};
//
//     printf("starting %d jobs...\n", N);
//     for (int i = 0; i < N; ++i) {
//         tasks[i].id = i;
//         tasks[i].header_ptr = header;
//         tasks[i].target_ptr = target;
//         tasks[i].timestamp = header->timestamp + i;
//
//         tasks[i].nonce_min = 0;
//         // tasks[i].nonce_min = 274148111 - 1000;
//         // tasks[i].nonce_min = 662101787 - 1000;
//
//         tasks[i].nonce_max = 0xFFFF'FFFF;
//         // tasks[i].nonce_max = 50'000'000;
//         // tasks[i].nonce_max = 10'000'000;
//
//         rs[i] = -1;
//         thrd_create(ws + i, (int (*)(void *))solve, &tasks[i]);
//     }
//
//     for (int i = 0; i < N; ++i) {
//         thrd_join(ws[i], &rs[i]);
//     }
//
//     const long double mega_s = 1.e+6;
//     const long double nano_s = 1.e-9;
//     long double dt_sum = 0;
//     uint64_t n_sum = 0;
//
//     printf("finished.\n");
//
//     for (int i = 0; i < N; ++i) {
//         const worker_ctx_t *const t = &tasks[i];
//
//         long double dt = (t->TN.tv_sec - t->T0.tv_sec) +
//                          (nano_s * (t->TN.tv_nsec - t->T0.tv_nsec));
//
//         long double rate = 1. / dt * t->N / mega_s;
//
//         dt_sum += dt;
//         n_sum += t->N;
//
//         printf("job-%02d: %lu in %.02Lfs, rate: %.2LfM, res: %d\n", i, t->N,
//         dt,
//                rate, rs[i]);
//     }
//
//     printf("Summary:\n");
//     long double rate = 1. / dt_sum * n_sum / mega_s * N;
//     printf(" complete %lu in %.02Lfs, rate: %.2LfM\n", n_sum, dt_sum / N,
//     rate);
// }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    int wid;
    thrd_t thrd;
    void *task;
    void *pool;
    int (*task_work)(void *);
    int (*task_pool_get)(void *pool, int, void *task);
    int (*task_pool_put)(void *pool, int, void *task);
} worker_t;

int worker_proc(void *ctx) {
    worker_t *const w = ctx;

    while (1) {
        if (w->task_pool_get(w->pool, w->wid, w->task) == 0) {
            break;
        }
        if (w->task_work(w->task)) {
            w->task_pool_put(w->pool, w->wid, w->task);
        }
    }

    return 0;
}

nu_G_define_dca(worker_array_t, worker_t, workers, static);

typedef struct {
    worker_array_t workers;
    find_nonce_task_pool_t *task_pool;
    mtx_t mtx_tasks;
} worker_pool_t;

void worker_pool_init(worker_pool_t *pool, size_t size);
int worker_pool_get_task(worker_pool_t *pool, int wid, find_nonce_task_t *task);
int worker_pool_put_task(worker_pool_t *pool, int wid, find_nonce_task_t *task);

void worker_pool_init(worker_pool_t *pool, size_t size) {
    mtx_init(&pool->mtx_tasks, mtx_plain);

    pool->workers = workers_create();
    worker_t *const ws = workers_consume(&pool->workers, size);
    for (int i = 0; i < size; ++i) {
        worker_t *const w = ws + i;
        *w = (worker_t){.wid = i};
    }
}

int worker_pool_get_task(worker_pool_t *pool, int wid,
                         find_nonce_task_t *task) {
    int ok = 0;

    if (mtx_trylock(&pool->mtx_tasks) != thrd_success) {
        printf("[w:%02d]: waits...\n", wid);
        if (mtx_lock(&pool->mtx_tasks) != thrd_success) {
            printf("[w:%02d]: lock failed.\n", wid);
            return 0;
        }
        printf("[w:%02d]: ready.\n", wid);
    }

    if (find_nonce_next_task(pool->task_pool, task) == 0) {
        printf("[w:%02d]: no tasks left.\n", wid);
        ok = 0;
    }
    else {
        printf("[w:%02d]: t=%10u, n=[%10u,%10u] (%u)\n", wid,
               task->header.timestamp, task->nonce_min, task->nonce_max,
               task->nonce_max - task->nonce_min);

        ok = 1;
    }

    mtx_unlock(&pool->mtx_tasks);

    return ok;
}

int worker_pool_put_task(worker_pool_t *pool, int wid,
                         find_nonce_task_t *task) {
    fflush(stdout);
    printf("[w:%02d]: solved.\n", wid);
    bc_fprint_block_header(stdout, &task->header);
    fflush(stdout);
    exit(0);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
        printf("Usage: %s block.dat [target]\n", argv[0]);
        return 1;
    }

    const char *opt_filepath = argv[1];
    const char *opt_target = argc > 3 ? argv[2] : NULL;

    bc_block_header_t header = {0};
    bc_hash32_t target = {0};

    // load block file
    nu_buffer_t buf = nu_buffer_create();
    if (btcp_load_file(&buf, opt_filepath)) {
        printf("error: failed to read file '%s'\n", opt_filepath);
        return 1;
    }
    if (nu_dca_size(&buf) < sizeof(header)) {
        printf("error: file '%s' is tool small\n", opt_filepath);
        nu_dca_release(&buf);
        return 1;
    }

    // copy block file
    header = *(const bc_block_header_t *)(nu_dca_data(&buf));
    nu_dca_release(&buf);

    // parse/construct target
    if (opt_target) {
        if (strlen(opt_target) != BcHashStringSize) {
            printf("error: target is not valid [%s]\n", opt_target);
            return 1;
        }
        bc_hash32_from_cspan(&target, nu_cspan_from_cstr(opt_target));
    }
    else {
        uint32_t bits_exp = (header.bits & 0xFF00'0000) >> 24;
        uint32_t bits_cff = (header.bits & 0x00FF'FFFF);
        memset(target.data, 0, sizeof(target.data));
        target.data[bits_exp - 3] = (bits_cff >> 0) & 0xFF;
        target.data[bits_exp - 2] = (bits_cff >> 8) & 0xFF;
        target.data[bits_exp - 1] = (bits_cff >> 16) & 0xFF;
    }

    printf("BlockHeader:\n");
    bc_fprint_block_header(stdout, &header);
    bc_fprint_field_hash32(stdout, "Target", &target);
    printf("\n");

    // initialize pool

    const int N = 20;
    const int T = 0;

    find_nonce_task_pool_t task_pool = {
        .header = header,
        .target = target,
        .nonce_chunks = N,
        .timestamp = header.timestamp,
        .timestamp_max = header.timestamp + 0,
    };

    nu_G_dca(find_nonce_task_t) task_cache = {.mem = nu_memory_get_default()};
    find_nonce_task_t *const ts = nu_dca_consume(&task_cache, N);

    worker_pool_t worker_pool = {0};
    worker_pool_init(&worker_pool, N);
    worker_pool.task_pool = &task_pool;

    // start workers

    printf("start: nw=%d, nt=%d...\n", N, T + 1);

    for (size_t i = 0; i < workers_size(&worker_pool.workers); ++i) {
        worker_t *const w = workers_begin(&worker_pool.workers) + i;
        w->task = ts + i;
        w->pool = &worker_pool;
        w->task_work = (int (*)(void *))find_nonce;
        w->task_pool_get = (int (*)(void *, int, void *))worker_pool_get_task;
        w->task_pool_put = (int (*)(void *, int, void *))worker_pool_put_task;
        thrd_create(&w->thrd, worker_proc, w);
    }

    for (size_t i = 0; i < workers_size(&worker_pool.workers); ++i) {
        worker_t *const w = workers_begin(&worker_pool.workers) + i;
        thrd_join(w->thrd, NULL);
    }

    return 0;
}
