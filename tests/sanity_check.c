#include "./testsuite.h"

#include <nulib/memory.h>

int main(void) {
    nulib_testsuite_init();

    {
        void *p = nu_calloc(1, 10);
        nu_check(p);
        nu_free(p);
    }
    {
        void *p = nu_calloc(0x0001ffff, 0xffffffffffff);
        nu_check(p == NULL);
    }

    return 0;
}
