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

#if !defined(NDEBUG)
#    define BP_EXIT()             \
        do {                      \
            asm volatile("int3"); \
            exit(EXIT_FAILURE);   \
        } while (0)
#else
#    define BP_EXIT()           \
        do {                    \
            exit(EXIT_FAILURE); \
        } while (0)
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

#if ENABLE_LOGGING
#    define LOG(msg, ...)                                                                                \
        do {                                                                                             \
            fprintf(stdout, "Log [%s:%s:%d] :: " msg "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__); \
        } while (0)
#else
#    define LOG(msg, ...)
#endif
