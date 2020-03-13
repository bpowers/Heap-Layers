// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil -*-
// Copyright 2020 The Heap-Layers Authors. All rights reserved.
// Use of this source code is governed by the Apache License,
// Version 2.0, that can be found in the COPYING file.

#pragma once
#ifndef HL_WRAPPER_COMMON_H
#define HL_WRAPPER_COMMON_H

#include <errno.h>

#include "../heaplayers-common.h"

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

// Takes a pointer and returns how much space it holds.
size_t HL_PREFIX(malloc_usable_size)(void *);

// its possible for a library to implement realloc directly
#ifdef HL_HAVE_REALLOC
void HL_PREFIX(realloc)(void *, size_t);
#else
// a single realloc implementation to rule them all
inline void *HL_PREFIX(realloc)(void *oldPtr, size_t newSize) {
  if (oldPtr == nullptr) {
    return HL_PREFIX(malloc)(newSize);
  }

  // 0 size = free. We return a small object.  This behavior is
  // apparently required under Mac OS X and optional under POSIX.
  if (newSize == 0) {
    HL_PREFIX(free)(oldPtr);
    return HL_PREFIX(malloc)(1);
  }

  auto oldSize = HL_PREFIX(malloc_usable_size)(ptr);

  // Custom logic here to ensure we only do a logarithmic number of
  // reallocations (with a constant space overhead).

  // Don't change size if the object is shrinking by less than half.
  const size_t upperBoundToShrink = oldSize / 2UL;
  if ((newSize > upperBoundToShrink) && (newSize <= oldSize)) {
    return oldPtr;
  }

  // always grow by at least 1.25x, this ensures we don't do
  // pathalogically bad if we keep resizing an object by (say) 1 byte.
  // The original Heap-Layers implemenation had a factor of 2x but was
  // commented out, this is less aggressive.
  const size_t lowerBoundToGrow = oldSize + oldSize / 4ul;
  newSize = newSize < lowerBoundToGrow ? lowerBoundToGrow : newSize;

  auto *newPtr = HL_PREFIX(malloc)(newSize);
  if (HL_UNLIKELY(newPtr == nullptr)) {
    return nullptr;
  }

  const size_t copySize = (oldSize < newSize) ? oldSize : newSize;
  memcpy(newPtr, oldPtr, copySize);

  return newPtr;
}
#endif

#ifdef HL_HAVE_CALLOC
void *HL_PREFIX(calloc)(size_t count, size_t size);
#else
inline void *HL_PREFIX(calloc)(size_t count, size_t size) {
  if (HL_UNLIKELY(size && count > (size_t)-1 / size)) {
    errno = ENOMEM;
    return nullptr;
  }

  const size_t n = count * size;
  auto *ptr = HL_PREFIX(malloc)(n);
  if (ptr != nullptr) {
    memset(ptr, 0, n);
  }

  return ptr;
}
#endif

// Pending widespread support for sized deallocation.
// void HL_PREFIX(free_sized)(void *, size_t);

// Locks the heap(s), used prior to any invocation of fork().
void HL_PREFIX(malloc_lock)();

// Unlocks the heap(s), after fork().
void HL_PREFIX(malloc_unlock)();
}

#endif  // HL_WRAPPER_COMMON_H
