#include "./testsuite.h"

#include <nulib/dca.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int test_basic() {
    typedef nu_G_dca(int) int_dca_t;

    int_dca_t ints = (int_dca_t){.mem = nu_memory_get_default()};

    const int val32 = 32;
    int *p;
    int *q;

    p = nu_dca_consume(&ints, 0);
    nu_check(p != NULL);
    nu_check(nu_dca_size(&ints) == 0);

    p = nu_dca_consume(&ints, 3);
    nu_check(p != NULL);
    nu_check(nu_dca_size(&ints) == 3);

    p = nu_dca_consume(&ints, 3);
    q = nu_dca_data(&ints);
    nu_check(p != NULL);
    nu_check(nu_dca_size(&ints) == 6);
    p[1] = val32;
    nu_check(q[4] == p[1]);

    nu_dca_reserve(&ints, 0);
    nu_check(nu_dca_size(&ints) == 6);
    nu_dca_reserve(&ints, 3);
    nu_check(nu_dca_size(&ints) == 6);
    nu_dca_reserve(&ints, 6);
    nu_check(nu_dca_size(&ints) == 6);

    q = nu_dca_reserve(&ints, 9);
    nu_check(q[4] == val32);

    q = nu_dca_reserve(&ints, 128);
    nu_check(q[4] == val32);

    nu_dca_release(&ints);
    nu_check(nu_dca_data(&ints) == NULL);
    nu_check(nu_dca_capacity(&ints) == 0);
    nu_check(nu_dca_size(&ints) == 0);

    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

nu_G_define_dca_type(my_string_t, char);
nu_G_define_dca_full_api(my_string_t, mystr);

int test_full_type() {
    my_string_t str = mystr_create();

    const char *p1 = mystr_reserve(&str, 1);
    nu_check(mystr_size(&str) == 0);

    const char *p2 = mystr_extend(&str, 2);
    nu_check(mystr_size(&str) == 0);

    const char *p3 = mystr_consume(&str, 3);
    nu_check(mystr_capacity(&str) != 0);
    nu_check(mystr_size(&str) == 3);
    nu_check(mystr_data(&str) == p3);

    mystr_clean(&str);
    nu_check(mystr_size(&str) == 0);
    nu_check(mystr_capacity(&str) != 0);

    mystr_release(&str);
    nu_check(mystr_capacity(&str) == 0);
    nu_check(mystr_data(&str) == NULL);

    return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int main(void) {
    nulib_testsuite_init();
    int ec = 0;
    ec += test_basic();
    ec += test_full_type();
    return ec;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
