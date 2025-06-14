// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#ifndef __LIBCORE_HPP
#define __LIBCORE_HPP

#include <LibCore/version.h>
#include <filesystem>
#include <string>

// Public API

namespace dotname {

  class LibCore {

    const std::string libName_ = std::string ("LibCore v.") + LIBCORE_VERSION;

  public:
    LibCore ();
    LibCore (const std::filesystem::path& assetsPath);
    ~LibCore ();
  };

} // namespace dotname

#endif // __LIBCORE_HPP