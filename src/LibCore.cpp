// MIT License
// Copyright (c) 2024-2025 Tomáš Mark

#include <LibCore/LibCore.hpp>
#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <Utils/Utils.hpp>
#include <Gui/PlatformManager.hpp>

#if defined(PLATFORM_WEB)
  #include <emscripten/emscripten.h>
#endif

namespace dotname {

  LibCore::LibCore () {
    LOG_D_STREAM << libName_ << " constructed ..." << std::endl;
    AssetContext::clearAssetsPath ();
  }

  LibCore::LibCore (const std::filesystem::path& assetsPath) : LibCore () {
    if (!assetsPath.empty ()) {
      AssetContext::setAssetsPath (assetsPath);
      LOG_D_STREAM << "Assets path given to the library\n"
                   << "╰➤ " << AssetContext::getAssetsPath () << std::endl;
      auto logo = std::ifstream (AssetContext::getAssetsPath () / "logo.png");
      initializePlatform ();
    }
  }

  LibCore::~LibCore () {
    LOG_D_STREAM << libName_ << " ... destructed" << std::endl;
  }

} // namespace dotname