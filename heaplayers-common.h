// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil -*-
// Copyright 2020 The Heap-Layers Authors. All rights reserved.
// Use of this source code is governed by the Apache License,
// Version 2.0, that can be found in the COPYING file.

#pragma once
#ifndef HL_HEAPLAYERS_COMMON_H
#define HL_HEAPLAYERS_COMMON_H

namespace HL {
// Good for practically all platforms.
static constexpr size_t kPageSize = 4096;
static constexpr size_t kCachelineSize = 64;
}  // namespace HL

// Define HL_EXECUTABLE_HEAP as 1 if you want that (i.e., you're doing
// dynamic code generation).
#ifndef HL_EXECUTABLE_HEAP
#define HL_EXECUTABLE_HEAP 0
#endif

// Microsoft Visual Studio
#if defined(_MSC_VER)

#pragma inline_depth(255)
#define HL_INLINE __forceinline
#define HL_NOINLINE __declspec(noinline)
#pragma warning(disable : 4530)
#define HL_MALLOC_FUNCTION
#define HL_RESTRICT

#define HL_ATTRIBUTE_ALWAYS_INLINE
#define HL_ATTRIBUTE_NEVER_INLINE
#define HL_ATTRIBUTE_INITIAL_EXEC
#define HL_ATTRIBUTE_NORETURN
#define HL_ATTRIBUTE_ALIGNED(s)
#define HL_CACHELINE_ALIGNED
#define HL_PAGE_ALIGNED
#define HL_EXPORT

#define HL_LIKELY(x) x
#define HL_UNLIKELY(x) x

// gcc and clang
#elif defined(__GNUC__) || defined(__clang__)

#define HL_INLINE inline
#define HL_NOINLINE __attribute__((noinline))
#define HL_MALLOC_FUNCTION __attribute__((malloc))
#define HL_RESTRICT __restrict__

#define HL_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))
#define HL_ATTRIBUTE_NEVER_INLINE __attribute__((noinline))
#define HL_ATTRIBUTE_INITIAL_EXEC __attribute__((tls_model("initial-exec")))
#define HL_ATTRIBUTE_NORETURN __attribute__((noreturn))
#define HL_ATTRIBUTE_ALIGNED(s) __attribute__((aligned(s)))
#define HL_CACHELINE_ALIGNED HL_ATTRIBUTE_ALIGNED(HL::kCachelineSize)
#define HL_PAGE_ALIGNED HL_ATTRIBUTE_ALIGNED(HL::kPageSize)
#define HL_EXPORT __attribute__((visibility("default")))

#define HL_LIKELY(x) __builtin_expect(!!(x), 1)
#define HL_UNLIKELY(x) __builtin_expect(!!(x), 0)

// All others
#else

#define HL_NOINLINE
#define HL_INLINE inline
#define HL_MALLOC_FUNCTION
#define HL_RESTRICT

#define HL_ATTRIBUTE_ALWAYS_INLINE
#define HL_ATTRIBUTE_NEVER_INLINE
#define HL_ATTRIBUTE_INITIAL_EXEC
#define HL_ATTRIBUTE_NORETURN
#define HL_ATTRIBUTE_ALIGNED(s)
#define HL_CACHELINE_ALIGNED
#define HL_PAGE_ALIGNED
#define HL_EXPORT

#define HL_LIKELY(x) x
#define HL_UNLIKELY(x) x

#endif  // defined(_MSC_VER)

// HL_HAVE_TLS is defined to 1 when __thread should be supported.  We
// assume __thread is supported on Linux when compiled with Clang or
// compiled against libstdc++ with _GLIBCXX_HAVE_TLS defined. (this
// check is from Abseil)
#ifdef HL_HAVE_TLS
#error HL_HAVE_TLS cannot be directly set
#elif defined(__linux__) && (defined(__clang__) || defined(_GLIBCXX_HAVE_TLS))
#define HL_HAVE_TLS 1
#endif

#endif  // HL_HEAPLAYERS_COMMON_H
