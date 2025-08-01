// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include <CoreLib/CoreLib.hpp>
#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <Utils/Utils.hpp>

#include <Gui/PlatformManager.hpp>

#if defined(__EMSCRIPTEN__)
  #include <emscripten/emscripten.h>
  #include "Gui/EmscriptenPlatform.hpp"
#else
  #include "Gui/DesktopPlatform.hpp"
#endif

namespace dotname {

  CoreLib::CoreLib () {
    LOG_D_STREAM << libName_ << " constructed ..." << std::endl;
    AssetContext::clearAssetsPath ();
  }

  CoreLib::CoreLib (const std::filesystem::path& assetsPath) : CoreLib () {
    if (!assetsPath.empty ()) {
      AssetContext::setAssetsPath (assetsPath);
      LOG_D_STREAM << "Assets: " << AssetContext::getAssetsPath () << std::endl;
      LOG_I_STREAM << DotNameUtils::JsonUtils::getCustomStringSign () << std::endl;
      auto logo = std::ifstream (AssetContext::getAssetsPath () / "logo.png");

#if defined(__EMSCRIPTEN__)
      static EmscriptenPlatform pltf;
      pltf.initialize ();
#else
      static DesktopPlatform pltf;
      pltf.initialize ();
#endif
    }
  }

  CoreLib::~CoreLib () {
    LOG_D_STREAM << libName_ << " ... destructed" << std::endl;
  }

} // namespace dotname