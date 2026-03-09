#include "bc_hex.h"
#include "bc_utils.h"
#include "sha256.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_sha256(const char *msg, size_t len, const char *sum) {
    uint8_t hash[32] = {0};
    char hex[65] = {0};
    sha256_digest(msg, len, hash);
    bc_write_hex_forward(hex, 65, hash, 32);

    int ec = memcmp(hex, sum, 64);
    if (ec) {
        printf("[%s] vs [%s] - msg:[%s]\n", hex, sum, msg);
    }
    return ec;
}

int test_hash256(const char *msg, size_t len, const char *sum) {
    uint8_t hash[32] = {0};
    char hex[65] = {0};
    hash256_digest(msg, len, hash);
    bc_write_hex_forward(hex, 65, hash, 32);

    int ec = memcmp(hex, sum, 64);
    if (ec) {
        printf("[%s] vs [%s] - msg:[%s]\n", hex, sum, msg);
    }
    return ec;
}

int main() {
    size_t num_failed = 0;

    typedef struct {
        const char *msg;
        const char *sum;
    } test_case_t;

    // SHA-256

    // const uint8_t Sha256_1[] = {0x5d, 0x5b, 0x09, 0xf6, 0xdc, 0xb2, 0xd5,
    // 0x3a,
    //                             0x5f, 0xff, 0xc6, 0x0c, 0x4a, 0xc0, 0xd5,
    //                             0x5f, 0xab, 0xdf, 0x55, 0x60, 0x69, 0xd6,
    //                             0x63, 0x15, 0x45, 0xf4, 0x2a, 0xa6, 0xe3,
    //                             0x50, 0x0f, 0x2e};
    // const uint8_t Sha256_2[] = {0x20, 0xa3, 0x0e, 0xab, 0xfa, 0xa2, 0x9a,
    // 0xff,
    //                             0x86, 0x6c, 0xe4, 0x71, 0xdd, 0x7b, 0xc1,
    //                             0x29, 0xcb, 0x3c, 0x36, 0x8b, 0xc4, 0x94,
    //                             0x09, 0xa4, 0x99, 0xa1, 0xfb, 0xd5, 0x85,
    //                             0x1b, 0x36, 0xcb};

    test_case_t sha256_cases[] = {
        {"sha256",
         "5d5b09f6dcb2d53a5fffc60c4ac0d55fabdf556069d6631545f42aa6e3500f2e"},

        {"5d5b09f6dcb2d53a5fffc60c4ac0d55fabdf556069d6631545f42aa6e3500f2e",
         "7b4ac9b4d6a0364f5f4f1063f41f858beb5167cf3bac39a781fbb1ef44c17804"},

        // msglen 0
        {"",
         "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},

        // msglen > 64
        {"0000002070d0fcfb0bfbb6230773c526898ee638c869478c138d01000000000000000"
         "00084d842146ee1e9288233eaad902b5b9b36e2430adf971302663986e35106491301"
         "e0a9690000000001000000",
         "6c45bde2ee704134491e21a5761cd28340274fa566bf5718ae6b7d40846092cd"},
    };

    const size_t sha256_cases_size =
        sizeof(sha256_cases) / sizeof(*sha256_cases);

    for (size_t i = 0; i < sha256_cases_size; ++i) {
        int ec = test_sha256(sha256_cases[i].msg, strlen(sha256_cases[i].msg),
                             sha256_cases[i].sum);
        if (ec) {
            num_failed += 1;
        }
    }

    // HASH-256

    test_case_t hash256_cases[] = {
        {
            "sha256",
            "20a30eabfaa29aff866ce471dd7bc129cb3c368bc49409a499a1fbd5851b36cb",
        },
    };

    const size_t hash256_cases_size =
        sizeof(hash256_cases) / sizeof(*hash256_cases);

    for (size_t i = 0; i < hash256_cases_size; ++i) {
        int ec =
            test_hash256(hash256_cases[i].msg, strlen(hash256_cases[i].msg),
                         hash256_cases[i].sum);
        if (ec) {
            num_failed += 1;
        }
    }

    //

    // echo -n 000000007a84cad9b807f0312b83c26d4b04990827d2116c87fbce796eb2526a
    // | xxd -r -p | xxd -i
    const uint8_t H1[] = {0x00, 0x00, 0x00, 0x00, 0x7a, 0x84, 0xca, 0xd9,
                          0xb8, 0x07, 0xf0, 0x31, 0x2b, 0x83, 0xc2, 0x6d,
                          0x4b, 0x04, 0x99, 0x08, 0x27, 0xd2, 0x11, 0x6c,
                          0x87, 0xfb, 0xce, 0x79, 0x6e, 0xb2, 0x52, 0x6a};
    const uint8_t H2[] = {0x00, 0x00, 0x00, 0x00, 0x7b, 0x6d, 0x0c, 0xf4,
                          0x55, 0x27, 0x30, 0xbf, 0x83, 0x10, 0x03, 0xb5,
                          0xee, 0xab, 0xa1, 0x62, 0x8e, 0x8f, 0x22, 0xd4,
                          0x54, 0x52, 0x93, 0xf7, 0x04, 0x06, 0x12, 0x6d};
    {
        uint8_t R1[32];
        uint8_t R2[32];
        for (int i = 31; i >= 0; i--) {
            R1[i] = H1[31 - i];
            R2[i] = H2[31 - i];
        }

        int c = i256_compare(R1, R2);
        if (c != -5) {
            printf("FAILED: i256_compare(R1, R2) = %d\n", c);
            num_failed += 1;
        }
        c = i256_compare(R2, R1);
        if (c != +5) {
            printf("FAILED: i256_compare(R2, R1) = %d\n", c);
            num_failed += 1;
        }
    }

    return num_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
