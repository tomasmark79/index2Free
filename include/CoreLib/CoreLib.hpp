// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#ifndef __LIBCORELIB_HPP
#define __LIBCORELIB_HPP

#include <CoreLib/version.h>
#include <filesystem>
#include <string>

// Public API

namespace dotname {

  class CoreLib {

    const std::string libName_ = std::string ("CoreLib v.") + CORELIB_VERSION;

  public:
    CoreLib ();
    CoreLib (const std::filesystem::path& assetsPath);
    ~CoreLib ();
  };

} // namespace dotname

#endif // __LIBCORELIB_HPP