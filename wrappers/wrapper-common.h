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

// clang-format off
#define HL_MALLOC(x)             HL_PREFIX(malloc)(x)
#define HL_FREE(x)               HL_PREFIX(free)(x)
#define HL_CFREE(x)              HL_PREFIX(cfree)(x)
#define HL_REALLOC(x,y)          HL_PREFIX(realloc)(x,y)
#define HL_CALLOC(x,y)           HL_PREFIX(calloc)(x,y)
#define HL_MEMALIGN(x,y)         HL_PREFIX(memalign)(x,y)
#define HL_POSIX_MEMALIGN(x,y,z) HL_PREFIX(posix_memalign)(x,y,z)
#define HL_ALIGNED_ALLOC(x,y)    HL_PREFIX(aligned_alloc)(x,y)
#define HL_GETSIZE(x)            HL_PREFIX(malloc_usable_size)(x)
#define HL_GOODSIZE(x)           HL_PREFIX(malloc_good_size)(x)
#define HL_VALLOC(x)             HL_PREFIX(valloc)(x)
#define HL_PVALLOC(x)            HL_PREFIX(pvalloc)(x)
#define HL_RECALLOC(x,y,z)       HL_PREFIX(recalloc)(x,y,z)
#define HL_STRNDUP(s,sz)         HL_PREFIX(strndup)(s,sz)
#define HL_STRDUP(s)             HL_PREFIX(strdup)(s)
#define HL_GETCWD(b,s)           HL_PREFIX(getcwd)(b,s)
#define HL_GETENV(s)             HL_PREFIX(getenv)(s)
// clang-format on

#ifdef __linux__
// clang-format off
#define HL_MALLOPT(x,y)          HL_PREFIX(mallopt)(x,y)
#define HL_MALLOC_TRIM(s)        HL_PREFIX(malloc_trim)(s)
#define HL_MALLOC_STATS(a)       HL_PREFIX(malloc_stats)(a)
#define HL_MALLOC_GET_STATE(p)   HL_PREFIX(malloc_get_state)(p)
#define HL_MALLOC_SET_STATE(p)   HL_PREFIX(malloc_set_state)(p)
#define HL_MALLINFO(a)           HL_PREFIX(mallinfo)(a)
// clang-format on
#endif

// these are all implemented by end-library as described above
extern "C" {

// marking these as inline allows the compiler to inline their
// definitions into e.g. `operator new`, removing branches and
// improving performance.
void *HL_ATTRIBUTE_INLINE HL_ATTRIBUTE_EXPORT HL_PREFIX(malloc)(size_t);
void HL_ATTRIBUTE_INLINE HL_ATTRIBUTE_EXPORT HL_PREFIX(free)(void *);
size_t HL_ATTRIBUTE_INLINE HL_ATTRIBUTE_EXPORT HL_PREFIX(malloc_usable_size)(void *);

void HL_ATTRIBUTE_INLINE HL_ATTRIBUTE_EXPORT HL_PREFIX(realloc)(void *, size_t);
void *HL_ATTRIBUTE_INLINE HL_ATTRIBUTE_EXPORT HL_PREFIX(calloc)(size_t count, size_t size);

// Pending widespread support for sized deallocation.
// void HL_PREFIX(free_sized)(void *, size_t);

// Locks the heap(s), used prior to any invocation of fork().
void HL_PREFIX(malloc_lock)();

// Unlocks the heap(s), after fork().
void HL_PREFIX(malloc_unlock)();
}

// implementations can provide optimized versions of these, but
// the generic ones aren't too shabby.
extern "C" {

#ifndef HL_HAVE_REALLOC
// a single realloc implementation to rule them all
void *HL_PREFIX(realloc)(void *oldPtr, size_t newSize) {
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

#ifndef HL_HAVE_CALLOC
void *HL_PREFIX(calloc)(size_t count, size_t size) {
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
}

#endif  // HL_WRAPPER_COMMON_H
