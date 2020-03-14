// -*- C++ -*-

/**
 * @file   macwrapper.cpp
 * @brief  Replaces malloc family on Macs with custom versions.
 * @author Emery Berger <http://www.emeryberger.com>
 * @note   Copyright (C) 2010-2018 by Emery Berger, University of Massachusetts Amherst.
 */

#ifndef __APPLE__
#error "This file is for use on Mac OS only."
#endif

#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <malloc/malloc.h>

#include "wrapper-common.h"

//////////
//////////

// All replacement functions get the prefix "replace_".

extern "C" {

void *HL_PREFIX(replace_malloc)(size_t sz) {
  void *ptr = HL_MALLOC(sz);
  return ptr;
}

#if 0  // Disabled pending wider support for sized deallocation.
void HL_PREFIX(replace_free_sized)(void * ptr, size_t sz) {
  HL_FREE_SIZED(ptr, sz);
}
#endif

size_t HL_PREFIX(replace_malloc_usable_size)(void *ptr) {
  if (ptr == nullptr) {
    return 0;
  }
  return HL_MALLOC_USABLE_SIZE(ptr);
}

void HL_PREFIX(replace_free)(void *ptr) {
  HL_FREE(ptr);
}

size_t HL_PREFIX(replace_malloc_good_size)(size_t sz) {
  auto *ptr = HL_MALLOC(sz);
  auto objSize = HL_MALLOC_USABLE_SIZE(ptr);
  HL_FREE(ptr);
  return objSize;
}

void *HL_PREFIX(replace_realloc)(void *oldPtr, size_t newSize) {
  return HL_REALLOC(oldPtr, newSize);
}

void *HL_PREFIX(replace_reallocf)(void *oldPtr, size_t newSize) {
  auto newPtr = HL_REALLOC(oldPtr, newSize);
  if (newPtr == nullptr) {
    HL_FREE(oldPtr);
  }
  return newPtr;
}

void *HL_PREFIX(replace_calloc)(size_t count, size_t size) {
  return HL_CALLOC(count, size);
}

char *HL_PREFIX(replace_strdup)(const char *origStr) {
  char *newStr = nullptr;
  if (HL_LIKELY(s != nullptr)) {
    const auto len = strlen(s) + 1UL;
    newStr = reinterpret_cast<char *>(HL_MALLOC(len));
    if (HL_LIKELY(newStr != nullptr)) {
      memcpy(newStr, origStr, len);
    }
  }
  return newStr;
}

void *HL_PREFIX(replace_memalign)(size_t alignment, size_t size) {
  // Check for non power-of-two alignment, or mistake in size.
  if (alignment < alignof(max_align_t)) {
    alignment = alignof(max_align_t);
  }
  // Round up to next power of two.
  if (alignment & (alignment - 1)) {
    size_t a = alignof(max_align_t);
    while (a < alignment) {
      a <<= 1;
    }
    alignment = a;
  }
  // Try to just allocate an object of the requested size.
  // If it happens to be aligned properly, just return it.
  auto *ptr = HL_MALLOC(size);
  if (((size_t)ptr & ~(alignment - 1)) == (size_t)ptr) {
    // It is already aligned just fine; return it.
    return ptr;
  }
  // It was not aligned as requested: free the object.
  HL_FREE(ptr);

  // Force size to be a multiple of alignment.
  if (alignment < size) {
    size = size + alignment - (size % alignment);
  } else {
    size = alignment;
  }

  ptr = HL_MALLOC(size);
  // If the underlying malloc has "natural" alignment, this will work.
  if (((size_t)ptr & ~(alignment - 1)) == (size_t)ptr) {
    // It is already aligned just fine; return it.
    return ptr;
  }
  // It was not aligned as requested: free the object.
  HL_FREE(ptr);

  // Now get a big chunk of memory and align the object within it.
  // NOTE: this assumes that the underlying allocator will be able
  // to free the aligned object, or ignore the free request.
  auto *buf = HL_MALLOC(2 * alignment + size);
  auto *alignedPtr = (void *)(((size_t)buf + alignment - 1) & ~(alignment - 1));
  return alignedPtr;
}

#if 0
void *HL_PREFIX(replace_aligned_alloc)(size_t alignment, size_t size) {
  // Per the man page: "The function aligned_alloc() is the same as
  // memalign(), except for the added restriction that size should be
  // a multiple of alignment." Rather than check and potentially fail,
  // we just enforce this by rounding up the size, if necessary.
  size = size + alignment - (size % alignment);
  return HL_MEMALIGN(alignment, size);
}
#endif

int HL_PREFIX(replace_posix_memalign)(void **memptr, size_t alignment, size_t size) {
  // Check for non power-of-two alignment.
  if (HL_UNLIKELY((alignment == 0) || (alignment & (alignment - 1)))) {
    return EINVAL;
  }
  auto *ptr = HL_MEMALIGN(alignment, size);
  if (HL_UNLIKELY(ptr == nullptr)) {
    return ENOMEM;
  } else {
    *memptr = ptr;
    return 0;
  }
}

void *HL_PREFIX(replace_valloc)(size_t sz) {
  return HL_MEMALIGN(HL::kPageSize, sz);
}

void HL_PREFIX(replace_vfree)(void *ptr) {
  HL_FREE(ptr);
}
}

/////////
/////////

extern "C" {
// operator new
void *_Znwm(unsigned long);
void *_Znam(unsigned long);

// operator delete
void _ZdlPv(void *);
void _ZdaPv(void *);

// nothrow variants
// operator new nothrow
void *_ZnwmRKSt9nothrow_t();
void *_ZnamRKSt9nothrow_t();
// operator delete nothrow
void _ZdaPvRKSt9nothrow_t(void *);
void _ZdlPvRKSt9nothrow_t(void *);

void _malloc_fork_prepare();
void _malloc_fork_parent();
void _malloc_fork_child();
}

static malloc_zone_t theDefaultZone;

extern "C" {

malloc_zone_t *replace_malloc_create_zone(vm_size_t, unsigned) {
  //    auto zone = (malloc_zone_t *) replace_malloc(sizeof(malloc_zone_t));
  return nullptr;  // zone;
}

malloc_zone_t *replace_malloc_default_zone() {
  return &theDefaultZone;
}

malloc_zone_t *replace_malloc_default_purgeable_zone() {
  return &theDefaultZone;
}

void replace_malloc_destroy_zone(malloc_zone_t *) {
  // Do nothing.
}

kern_return_t replace_malloc_get_all_zones(task_t, memory_reader_t, vm_address_t **addresses, unsigned *count) {
  *addresses = 0;
  count = 0;
  return KERN_SUCCESS;
}

const char *replace_malloc_get_zone_name(malloc_zone_t *z) {
  return z->zone_name;
}

void replace_malloc_printf(const char *, ...) {
}

size_t replace_internal_malloc_zone_size(malloc_zone_t *, const void *ptr) {
  return replace_malloc_usable_size((void *)ptr);
}

int replace_malloc_jumpstart(int) {
  return 1;
}

void replace_malloc_set_zone_name(malloc_zone_t *, const char *) {
  // do nothing.
}

unsigned replace_malloc_zone_batch_malloc(malloc_zone_t *, size_t sz, void **results, unsigned num_requested) {
  for (auto i = 0U; i < num_requested; i++) {
    results[i] = replace_malloc(sz);
    if (results[i] == nullptr) {
      return i;
    }
  }
  return num_requested;
}

void replace_malloc_zone_batch_free(malloc_zone_t *, void **to_be_freed, unsigned num) {
  for (auto i = 0U; i < num; i++) {
    replace_free(to_be_freed[i]);
  }
}

void *replace_malloc_zone_calloc(malloc_zone_t *, size_t n, size_t size) {
  return replace_calloc(n, size);
}

bool replace_malloc_zone_check(malloc_zone_t *) {
  // Just return true for all zones.
  return true;
}

void replace_malloc_zone_free(malloc_zone_t *, void *ptr) {
  replace_free(ptr);
}

void replace_malloc_zone_free_definite_size(malloc_zone_t *, void *ptr, size_t) {
  replace_free(ptr);
}

malloc_zone_t *replace_malloc_zone_from_ptr(const void *) {
  return replace_malloc_default_zone();
}

void replace_malloc_zone_log(malloc_zone_t *, void *) {
  // Do nothing.
}

void *replace_malloc_zone_malloc(malloc_zone_t *, size_t size) {
  return replace_malloc(size);
}

void replace_malloc_zone_print(malloc_zone_t *, bool) {
  // Do nothing.
}

void replace_malloc_zone_print_ptr_info(void *) {
}

void *replace_malloc_zone_realloc(malloc_zone_t *, void *ptr, size_t size) {
  return replace_realloc(ptr, size);
}

void replace_malloc_zone_register(malloc_zone_t *) {
}

void *replace_malloc_zone_memalign(malloc_zone_t *, size_t alignment, size_t size) {
  return replace_memalign(alignment, size);
}

void replace_malloc_zone_unregister(malloc_zone_t *) {
}

void *replace_malloc_zone_valloc(malloc_zone_t *, size_t size) {
  return replace_valloc(size);
}

void replace__malloc_fork_child() {
  /* Called in the child process after a fork() to resume normal operation.  In the MTASK case we also have to change
   * memory inheritance so that the child does not share memory with the parent. */
  xxmalloc_unlock();
}

void replace__malloc_fork_parent() {
  /* Called in the parent process after a fork() to resume normal operation. */
  xxmalloc_unlock();
}

void replace__malloc_fork_prepare() {
  /* Prepare the malloc module for a fork by insuring that no thread is in a malloc critical section */
  xxmalloc_lock();
}
}

extern "C" void vfree(void *);
extern "C" int malloc_jumpstart(int);

// Now interpose everything.

#define HL_REPLACE_ZONES 0

// The interposition data structure (just pairs of function pointers),
// used an interposition table like the following:
typedef struct interpose_s {
  void *new_func;
  void *orig_func;
} interpose_t;

#define MAC_INTERPOSE(newf, oldf)                                         \
  __attribute__((used)) static const interpose_t macinterpose##newf##oldf \
      __attribute__((section("__DATA, __interpose"))) = {(void *)newf, (void *)oldf}

MAC_INTERPOSE(replace__malloc_fork_child, _malloc_fork_child);
MAC_INTERPOSE(replace__malloc_fork_parent, _malloc_fork_parent);
MAC_INTERPOSE(replace__malloc_fork_prepare, _malloc_fork_prepare);
// MAC_INTERPOSE(replace_aligned_alloc, aligned_alloc);
MAC_INTERPOSE(replace_calloc, calloc);
MAC_INTERPOSE(replace_free, _ZdaPv);
MAC_INTERPOSE(replace_free, _ZdaPvRKSt9nothrow_t);
MAC_INTERPOSE(replace_free, _ZdlPv);
MAC_INTERPOSE(replace_free, _ZdlPvRKSt9nothrow_t);
MAC_INTERPOSE(replace_free, free);
MAC_INTERPOSE(replace_free, vfree);
MAC_INTERPOSE(replace_malloc, _Znam);
MAC_INTERPOSE(replace_malloc, _ZnamRKSt9nothrow_t);
MAC_INTERPOSE(replace_malloc, _Znwm);
MAC_INTERPOSE(replace_malloc, _ZnwmRKSt9nothrow_t);
MAC_INTERPOSE(replace_malloc, malloc);

#if HL_REPLACE_ZONES
MAC_INTERPOSE(replace_malloc_create_zone, malloc_create_zone);
MAC_INTERPOSE(replace_malloc_default_purgeable_zone, malloc_default_purgeable_zone);
MAC_INTERPOSE(replace_malloc_default_zone, malloc_default_zone);
MAC_INTERPOSE(replace_malloc_destroy_zone, malloc_destroy_zone);
MAC_INTERPOSE(replace_malloc_get_all_zones, malloc_get_all_zones);
MAC_INTERPOSE(replace_malloc_get_zone_name, malloc_get_zone_name);
#endif

MAC_INTERPOSE(replace_malloc_good_size, malloc_good_size);

#if HL_REPLACE_ZONES
MAC_INTERPOSE(replace_malloc_jumpstart, malloc_jumpstart);
#endif

MAC_INTERPOSE(replace_malloc_printf, malloc_printf);
MAC_INTERPOSE(replace_malloc_set_zone_name, malloc_set_zone_name);
MAC_INTERPOSE(replace_malloc_usable_size, malloc_size);

#if HL_REPLACE_ZONES
MAC_INTERPOSE(replace_malloc_zone_batch_free, malloc_zone_batch_free);
MAC_INTERPOSE(replace_malloc_zone_batch_malloc, malloc_zone_batch_malloc);
MAC_INTERPOSE(replace_malloc_zone_calloc, malloc_zone_calloc);
MAC_INTERPOSE(replace_malloc_zone_check, malloc_zone_check);
MAC_INTERPOSE(replace_malloc_zone_free, malloc_zone_free);
MAC_INTERPOSE(replace_malloc_zone_from_ptr, malloc_zone_from_ptr);
MAC_INTERPOSE(replace_malloc_zone_log, malloc_zone_log);
MAC_INTERPOSE(replace_malloc_zone_malloc, malloc_zone_malloc);
MAC_INTERPOSE(replace_malloc_zone_memalign, malloc_zone_memalign);
MAC_INTERPOSE(replace_malloc_zone_print, malloc_zone_print);
MAC_INTERPOSE(replace_malloc_zone_print_ptr_info, malloc_zone_print_ptr_info);
MAC_INTERPOSE(replace_malloc_zone_realloc, malloc_zone_realloc);
MAC_INTERPOSE(replace_malloc_zone_register, malloc_zone_register);
MAC_INTERPOSE(replace_malloc_zone_unregister, malloc_zone_unregister);
MAC_INTERPOSE(replace_malloc_zone_valloc, malloc_zone_valloc);
#endif
MAC_INTERPOSE(replace_posix_memalign, posix_memalign);
MAC_INTERPOSE(replace_realloc, realloc);
MAC_INTERPOSE(replace_reallocf, reallocf);
MAC_INTERPOSE(replace_strdup, strdup);
MAC_INTERPOSE(replace_valloc, valloc);

// A class to initialize exactly one malloc zone with the calls used
// by our replacement.

static const char *theOneTrueZoneName = "DefaultMallocZone";

class initializeDefaultZone {
public:
  initializeDefaultZone() {
    theDefaultZone.size = replace_internal_malloc_zone_size;
    theDefaultZone.malloc = replace_malloc_zone_malloc;
    theDefaultZone.calloc = replace_malloc_zone_calloc;
    theDefaultZone.valloc = replace_malloc_zone_valloc;
    theDefaultZone.free = replace_malloc_zone_free;
    theDefaultZone.realloc = replace_malloc_zone_realloc;
    theDefaultZone.destroy = replace_malloc_destroy_zone;
    theDefaultZone.zone_name = theOneTrueZoneName;
    theDefaultZone.batch_malloc = replace_malloc_zone_batch_malloc;
    theDefaultZone.batch_free = replace_malloc_zone_batch_free;
    theDefaultZone.introspect = NULL;
    theDefaultZone.version = 8;
    theDefaultZone.memalign = replace_malloc_zone_memalign;
    theDefaultZone.free_definite_size = replace_malloc_zone_free_definite_size;
    theDefaultZone.pressure_relief = NULL;
    // Unregister and reregister the default zone.  Unregistering swaps
    // the specified zone with the last one registered which for the
    // default zone makes the more recently registered zone the default
    // zone.  The default zone is then re-registered to ensure that
    // allocations made from it earlier will be handled correctly.
    // Things are not guaranteed to work that way, but it's how they work now.
    malloc_zone_t *default_zone = malloc_default_zone();
    malloc_zone_unregister(default_zone);
    malloc_zone_register(&theDefaultZone);
  }
};

// Force initialization of the default zone.

#if REPLACE_ZONES
static initializeDefaultZone initMe;
#endif
