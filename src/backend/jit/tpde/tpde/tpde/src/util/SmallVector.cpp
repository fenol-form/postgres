// SPDX-FileCopyrightText: 2025 Contributors to TPDE <https://tpde.org>
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "tpde/util/SmallVector.hpp"

#include "tpde/base.hpp"
#include "tpde/util/AddressSanitizer.hpp"

#include <cstdlib>
#include <cstring>

namespace tpde::util {

static size_t calc_new_capacity(size_t cap, size_t min_size) {
  size_t new_cap = 2 * cap + 1;
  return new_cap < min_size ? min_size : new_cap;
}

void *SmallVectorUntypedBase::grow_malloc(size_type min_size,
                                          size_type elem_sz,
                                          size_type &new_cap) {
  new_cap = calc_new_capacity(cap, min_size);
  void *new_alloc = std::malloc(new_cap * elem_sz);
  if (!new_alloc) {
    TPDE_FATAL("SmallVector allocation failed");
  }
  return new_alloc;
}

void SmallVectorUntypedBase::grow_trivial(size_type min_size,
                                          size_type elem_sz) {
  size_type new_cap = calc_new_capacity(cap, min_size);
  void *new_alloc;
  if (is_small()) {
    new_alloc = std::malloc(new_cap * elem_sz);
    if (!new_alloc) {
      TPDE_FATAL("SmallVector allocation failed");
    }
    if (sz > 0) {
      std::memcpy(new_alloc, ptr, sz * elem_sz);
    }
    poison_memory_region(ptr, cap * elem_sz);
  } else {
    new_alloc = std::realloc(ptr, new_cap * elem_sz);
    if (!new_alloc) {
      TPDE_FATAL("SmallVector allocation failed");
    }
  }
  ptr = new_alloc;
  cap = new_cap;
  poison_memory_region((char *)ptr + sz * elem_sz, (cap - sz) * elem_sz);
}

} // end namespace tpde::util
