#ifndef _NULIB_TESTSUITE_H_INCLUDED_
#define _NULIB_TESTSUITE_H_INCLUDED_

#include <nulib/assert.h>

static inline void nulib_testsuite_init(void) {
    nulib_default_assert_failure_handler =
        _nulib_stmt_check_failure_report_print_and_exit;

    nulib_default_expect_failure_handler =
        _nulib_stmt_check_failure_report_print;

    nulib_default_ensure_failure_handler =
        _nulib_stmt_check_failure_report_print;
}

#define _nulib_emit_check_failure(stmt_str)                                    \
    (_nulib_emit_stmt_check_failure(                                           \
        _nulib_stmt_check_failure_report_print_and_exit, stmt_str,             \
        "Test Check"))

#define nu_check(stmt)                                                         \
    do {                                                                       \
        if (!(stmt)) {                                                         \
            _nulib_emit_check_failure(#stmt);                                  \
            return 1;                                                          \
        }                                                                      \
    } while (0)

#endif
