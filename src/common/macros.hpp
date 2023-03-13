#pragma once

#include <assert.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
#    include <type_traits>
#endif

#include "common/config.hpp"

#define static_assert_decl(const_expr) static_assert((const_expr), "Assertion false: " #const_expr)
#ifndef __cplusplus
#    define static_assert_expr(const_expr) (0 * sizeof(struct { static_assert_decl(const_expr); }))
#else
#    define static_assert_expr(const_expr)              \
        (                                               \
            []() {                                      \
                static_assert(const_expr, #const_expr); \
            },                                          \
            0)
#endif

#ifndef __cplusplus
#    define same_type(a, b) (__builtin_types_compatible_p(__typeof__(a), __typeof__(b)))
#else
#    define same_type(a, b) (std::is_same_v<decltype(a), decltype(b)>)
#endif
#define is_array(a) static_assert_expr(!same_type((a), &(a)[0]))

#define containerof(ptr, type, member) ((type*)((char*)ptr - offsetof(type, member)))
#define lengthof(arr)                  (std::size(arr))
#define likely(x)                      __builtin_expect(!!(x), 1)
#define unlikely(x)                    __builtin_expect(!!(x), 0)

#define OPTIMIZE_UNREACHABLE __builtin_unreachable()
#define OPTIMIZE_ASSUME(expr)     \
    do {                          \
        if (!(expr))              \
            OPTIMIZE_UNREACHABLE; \
    } while (0)

// TODO: could use __builtin_trap()
#if !defined(NDEBUG)
#    define BP_EXIT()             \
        do {                      \
            asm volatile("int3"); \
            exit(EXIT_FAILURE);   \
        } while (0)
#else
#    define BP_EXIT() exit(EXIT_FAILURE)
#endif

// TODO: check what this does in GDB, if it doesn't trigger a breakpoint at the abort call, use int3 or equiv
#define ABORT(msg, ...)               \
    do {                              \
        fprintf(                      \
            stderr,                   \
            "Aborted in %s @ %s:%d\n" \
            "Reason: " msg "\n\n",    \
            __func__,                 \
            __FILE__,                 \
            __LINE__,                 \
            ##__VA_ARGS__);           \
        BP_EXIT();                    \
    } while (0)

// TODO: overload, 2+ args provides msg
#define ASSERT(cond)                               \
    do {                                           \
        if (!(cond)) {                             \
            fprintf(                               \
                stderr,                            \
                "Assertion failed in %s @ %s:%d\n" \
                "Condition: %s\n\n",               \
                __func__,                          \
                __FILE__,                          \
                __LINE__,                          \
                #cond);                            \
            BP_EXIT();                             \
        }                                          \
    } while (0)

#define ANSI_START "\x1B["
#define ANSI_END   "m"

#define ANSI_FG_RED    ";31"
#define ANSI_FG_YELLOW ";33"
#define ANSI_FG_BLUE   ";34"

#define ANSI_BOLD  "1"
#define ANSI_FAINT "2"
#define ANSI_BLINK "5"

#define ANSI_RESET_ALL   "\x1B[0m"
#define ANSI(style, str) ANSI_START style ANSI_END str ANSI_RESET_ALL

#define LOCATION_STYLE ANSI_FAINT
#define ERROR_STYLE    ANSI_BOLD ANSI_FG_RED
#define WARNING_STYLE  ANSI_BOLD ANSI_FG_YELLOW
#define INFO_STYLE     ANSI_BOLD ANSI_FG_BLUE

// TODO: ability to log to file
#if ENABLE_LOGGING
#    define _LOG_INTERNAL(prefix, msg, ...)                               \
        fprintf(                                                          \
            stdout,                                                       \
            prefix ANSI(LOCATION_STYLE, " [%s:%d in '%s'] :: ") msg "\n", \
            __FILE__,                                                     \
            __LINE__,                                                     \
            __PRETTY_FUNCTION__,                                          \
            ##__VA_ARGS__)
#else
#    define _LOG_INTERNAL(prefix, msg, ...)
#endif

#define LOG_ERROR(msg, ...)   _LOG_INTERNAL(ANSI(ERROR_STYLE, "ERROR  "), msg, ##__VA_ARGS__)
#define LOG_WARNING(msg, ...) _LOG_INTERNAL(ANSI(WARNING_STYLE, "WARNING"), msg, ##__VA_ARGS__)
#define LOG_INFO(msg, ...)    _LOG_INTERNAL(ANSI(INFO_STYLE, "INFO   "), msg, ##__VA_ARGS__)
