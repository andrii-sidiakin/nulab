#ifndef _NULIB_ASSERT_H_INCLUDED_
#define _NULIB_ASSERT_H_INCLUDED_

#include "./preset.h"

#include <stdio.h>
#include <stdlib.h>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define NULIB_ASSERT_VERBOSE

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

typedef struct {
    const char *stmt;
    const char *what;
    const char *func;
    const char *file;
    int line;
} nulib_stmt_check_failure_report_t;

typedef void (*nulib_stmt_check_failure_handler_t)(
    nulib_stmt_check_failure_report_t);

extern nulib_stmt_check_failure_handler_t nulib_default_assert_failure_handler;
extern nulib_stmt_check_failure_handler_t nulib_default_expect_failure_handler;
extern nulib_stmt_check_failure_handler_t nulib_default_ensure_failure_handler;

NULIB_MaybeUnused static inline int _nulib_stmt_check_maybe_unsused(int x) {
    return x;
}

#define _nulib_emit_stmt_check_failure(handler, stmt_str, what_str)            \
    _nulib_stmt_check_maybe_unsused(                                           \
        (handler((nulib_stmt_check_failure_report_t){                          \
             .stmt = stmt_str,                                                 \
             .what = what_str,                                                 \
             .func = __func__,                                                 \
             .file = __FILE__,                                                 \
             .line = __LINE__,                                                 \
         }),                                                                   \
         0))

#define _nulib_emit_assert_failure(stmt_str)                                   \
    (_nulib_emit_stmt_check_failure(nulib_default_assert_failure_handler,      \
                                    stmt_str, "Assertion failure"))

#define _nulib_emit_expect_failure(stmt_str)                                   \
    (_nulib_emit_stmt_check_failure(nulib_default_expect_failure_handler,      \
                                    stmt_str, "Precondition failure"))

#define _nulib_emit_ensure_failure(stmt_str)                                   \
    (_nulib_emit_stmt_check_failure(nulib_default_ensure_failure_handler,      \
                                    stmt_str, "Postcondition failure"))

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define nu_assert(x)                                                           \
    do {                                                                       \
        if (!(x)) {                                                            \
            _nulib_emit_assert_failure(#x);                                    \
        }                                                                      \
    } while (0)

#define nu_expect(x)                                                           \
    _nulib_stmt_check_maybe_unsused((x) ? (1)                                  \
                                        : (_nulib_emit_expect_failure(#x), 0))

#define nu_ensure(x)                                                           \
    _nulib_stmt_check_maybe_unsused((x) ? (1)                                  \
                                        : (_nulib_emit_ensure_failure(#x), 0))

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static inline void
_nulib_stmt_check_failure_print_report(nulib_stmt_check_failure_report_t r) {
#ifdef NULIB_ASSERT_VERBOSE
    fprintf(stderr, "%s: [ %s ] in '%s', at '%s:%i'\n", r.what, r.stmt, r.func,
            r.file, r.line);
#else
    nu_unused(stmt);
    nu_unused(what);
    nu_unused(func);
    nu_unused(file);
    nu_unused(line);
#endif
}

NULIB_NoReturn static inline void
_nulib_stmt_check_failure_print_report_and_exit(
    nulib_stmt_check_failure_report_t r) {
    _nulib_stmt_check_failure_print_report(r);
    exit(EXIT_FAILURE);
}

NULIB_NoReturn static inline void
_nulib_stmt_check_failure_print_report_and_abort(
    nulib_stmt_check_failure_report_t r) {
    _nulib_stmt_check_failure_print_report(r);
    abort();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
