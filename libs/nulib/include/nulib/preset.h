#ifndef _NULIB_PRESET_H_INCLUDED_
#define _NULIB_PRESET_H_INCLUDED_

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// C std version

#if defined(__STDC_VERSION__)

#if __STDC_VERSION__ >= 199001L
#define NULIB_SUPPORTS_C99
#endif

#if __STDC_VERSION__ >= 201112L
#define NULIB_SUPPORTS_C11
#endif

#if __STDC_VERSION__ >= 201710L
#define NULIB_SUPPORT_C17
#endif

#if __STDC_VERSION__ >= 202311L
#define NULIB_SUPPORT_C23
#endif

#else

#if defined(__STDC__)
#define NULIB_SUPPORT_C90
#endif

#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define NULIB_API extern

#define NULIB_API_INLINE static inline

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define nu_unused(x) (void)(x)

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// nu_alignof

#ifdef NULIB_SUPPORTS_C23
#define nu_alignof alignof
#else
#if defined(_MSC_VER)
#define nu_alignof __alignof
#else
#define nu_alignof __alignof__
#endif
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// nu_typeof

#ifdef NULIB_SUPPORTS_C23
#define nu_typeof typeof
#else
#if defined(_MSC_VER)
#define nu_typeof __typeof__
#else
#define nu_typeof __typeof__
#endif
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// nu_constexpr

#ifdef NULIB_SUPPORTS_C23
#define nu_constexpr constexpr
#else
#define nu_constexpr const
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//

#ifdef NULIB_SUPPORTS_C23
#define NULIB_NoDiscard [[nodiscard]]
#define NULIB_NoReturn [[noreturn]]
#define NULIB_MaybeUnused [[noreturn]]
#else
#define NULIB_NoDiscard
#define NULIB_NoReturn
#define NULIB_MaybeUnused
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//

#if __has_include(<stdckdint.h>)
#define NULIB_HAS_CKD_OPS
#include <stdckdint.h>
#else 
// TODO: provide ckd ops
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#endif
