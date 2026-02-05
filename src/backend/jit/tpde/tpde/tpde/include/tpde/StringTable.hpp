// SPDX-FileCopyrightText: 2025 Contributors to TPDE <https://tpde.org>
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#pragma once

#include "tpde/util/SmallVector.hpp"

#include <cstddef>
#include <string_view>

namespace tpde {

class StringTable {
  util::SmallVector<char, 24> strtab;

public:
  StringTable() noexcept { strtab.resize(1); }

  size_t size() const noexcept { return strtab.size(); }
  const char *data() const noexcept { return strtab.data(); }

  size_t add(std::string_view str) noexcept;
  size_t add_prefix(std::string_view prefix, std::string_view str) noexcept;
};

} // namespace tpde
