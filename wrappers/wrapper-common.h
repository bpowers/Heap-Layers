// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil -*-
// Copyright 2019 The Heap-Layers Authors. All rights reserved.
// Use of this source code is governed by the Apache License,
// Version 2.0, that can be found in the COPYING file.

#pragma once
#ifndef HL_WRAPPER_COMMON_H
#define HL_WRAPPER_COMMON_H

// To use this library,
// you only need to define the following allocation functions:
//
// - xxmalloc
// - xxfree
// - xxmalloc_usable_size
// - xxmalloc_lock
// - xxmalloc_unlock
//
// You can replace the `xx` prefix with one of your choosing (like
// `hoard_` by setting `-DHL_EXPORT_PREFIX=hoard_`.
//
// See the extern "C" block below for function prototypes and more
// details. YOU SHOULD NOT NEED TO MODIFY ANY OF THE CODE HERE TO
// SUPPORT ANY ALLOCATOR.
//
// LIMITATIONS:
//
// - This wrapper assumes that the underlying allocator will do "the
//   right thing" when xxfree() is called with a pointer internal to an
//   allocated object. Header-based allocators, for example, need not
//   apply.
//
// - This wrapper also assumes that there is some way to lock all the
//   heaps used by a given allocator; however, such support is only
//   required by programs that also call fork(). In case your program
//   does not, the lock and unlock calls given below can be no-ops.

#ifndef HL_EXPORT_PREFIX
#define HL_EXPORT_PREFIX xx
#endif

#ifndef HL_PREFIX
#define HL_PREFIX(fn) HL_EXPORT_PREFIX##fn
#endif

// these are all implemented by end-library as described above
extern "C" {
void *HL_PREFIX(malloc)(size_t);
void HL_PREFIX(free)(void *);

// Pending widespread support for sized deallocation.
// void HL_PREFIX(free_sized)(void *, size_t);

// Takes a pointer and returns how much space it holds.
size_t HL_PREFIX(malloc_usable_size)(void *);

// Locks the heap(s), used prior to any invocation of fork().
void HL_PREFIX(malloc_lock)();

// Unlocks the heap(s), after fork().
void HL_PREFIX(malloc_unlock)();
}

#endif  // HL_WRAPPER_COMMON_H
