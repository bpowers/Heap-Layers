// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil -*-
// Copyright 2020 The Heap-Layers Authors. All rights reserved.
// Use of this source code is governed by the Apache License,
// Version 2.0, that can be found in the COPYING file.

#pragma once
#ifndef HL_HEAPLAYERS_COMMON_H
#define HL_HEAPLAYERS_COMMON_H

#if defined(__linux__) || defined(__APPLE__)
#define HL_EXPORT __attribute__((visibility("default")))
#else
#define HL_EXPORT
#endif

#define HL_ATTRIBUTE_NEVER_INLINE __attribute__((noinline))
#define HL_ATTRIBUTE_ALWAYS_INLINE __attribute__((always_inline))

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

// GNU C
#elif defined(__GNUC__)

#define HL_INLINE inline
#define HL_NOINLINE __attribute__((noinline))
#define HL_MALLOC_FUNCTION __attribute__((malloc))
#define HL_RESTRICT __restrict__

// All others
#else

#define HL_NOINLINE
#define HL_INLINE inline
#define HL_MALLOC_FUNCTION
#define HL_RESTRICT

#endif // defined(_MSC_VER)

#define HL_LIKELY(x) __builtin_expect(!!(x), 1)
#define HL_UNLIKELY(x) __builtin_expect(!!(x), 0)

#endif  // HL_HEAPLAYERS_COMMON_H
