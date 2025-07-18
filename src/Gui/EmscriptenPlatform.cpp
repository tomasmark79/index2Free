#include "EmscriptenPlatform.hpp"

#include "Logger/Logger.hpp"

#include <emscripten.h>
#include <emscripten/html5.h>

EM_JS (float, getDevicePixelRatio, (), { return window.devicePixelRatio || 1.0; });
EM_JS (int, getScreenWidth, (), { return screen.width; });
EM_JS (int, getScreenHeight, (), { return screen.height; });
EM_JS (int, getWindowWidth, (), { return window.innerWidth; });
EM_JS (int, getWindowHeight, (), { return window.innerHeight; });

EM_JS (bool, isTouchDevice, (),
       { return 'ontouchstart' in window || navigator.maxTouchPoints > 0; });
EM_JS (bool, isDocumentFullscreen, (), {
  return !!(document.fullscreenElement || document.webkitFullscreenElement
            || document.mozFullScreenElement || document.msFullscreenElement);
});
// clang-format off
EM_JS (bool, isMobileDevice, (), {
  return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
});
// clang-format on

// Function to get display information
EM_JS (void, getDisplayInfo, (float* devicePixelRatio, int* windowWidth, int* windowHeight), {
  setValue (devicePixelRatio, window.devicePixelRatio || 1.0, 'float');
  setValue (windowWidth, window.innerWidth, 'i32');
  setValue (windowHeight, window.innerHeight, 'i32');
});

struct EmscriptenDisplayInfo {
  float devicePixelRatio;
  int windowWidth, windowHeight;
  bool isScaled () const {
    return devicePixelRatio > 1.0f;
  }
  float getEffectiveScale () const {
    return isScaled () ? devicePixelRatio : 1.0f;
  }
  void update () {
    getDisplayInfo (&devicePixelRatio, &windowWidth, &windowHeight);
  }
};

void EmscriptenPlatform::initialize () {
  createSDL2Window ("Emscripten SDL2 Window", windowWidth_, windowHeight_);
  createOpenGLContext (1);
  setupQuad ();
  setupShaders ();
  initializeImGui ();
  updateWindowSize ();
  scaleImGui (this->userConfigurableScale_);
  initInputHandlerCallbacks ();
  emscripten_set_main_loop_arg (
      [] (void* userData) {
        EmscriptenPlatform* platform = static_cast<EmscriptenPlatform*> (userData);
        platform->mainLoop ();
      },
      this, 0, 1);
}

void EmscriptenPlatform::updateWindowSize () {
  int width, height;
  SDL_GL_GetDrawableSize (window_, &width, &height);
  if (width != windowWidth_ || height != windowHeight_) {
    windowWidth_ = width;
    windowHeight_ = height;
    devicePixelRatio_ = getDevicePixelRatio (); // JavaScript function to get device pixel ratio
    io_->DisplaySize = ImVec2 ((float)windowWidth_, (float)windowHeight_);
    io_->DisplayFramebufferScale = ImVec2 (devicePixelRatio_, devicePixelRatio_);
  }
}

std::string EmscriptenPlatform::getOverlayContent () {
  EmscriptenDisplayInfo lastDisplayInfo;
  lastDisplayInfo.update ();
  std::string oC = "";

  oC += fmt::format ("=== Overlay ===\n");
  oC += fmt::format ("Windows Width: {}\n", windowWidth_);
  oC += fmt::format ("Windows Height: {}\n", windowHeight_);
  oC += fmt::format ("Device Pixel Ratio: {:.2f}\n", devicePixelRatio_);
  oC += fmt::format ("Base Font Size: {:.2f}\n", BASE_FONT_SIZE);
  oC += fmt::format ("ImGui Display Size: {:.0f} x {:.0f}\n", io_->DisplaySize.x,
                     io_->DisplaySize.y);
  oC += fmt::format ("ImGui Display Framebuffer Scale: {:.2f} x {:.2f}\n",
                     io_->DisplayFramebufferScale.x, io_->DisplayFramebufferScale.y);
  oC += fmt::format ("=== Web Display Info ===\n");
  oC += fmt::format ("Browser Window: {}x{}\n", lastDisplayInfo.windowWidth,
                     lastDisplayInfo.windowHeight);
  oC += fmt::format ("Is Scaled Display: {}\n", lastDisplayInfo.isScaled () ? "YES" : "NO");
  oC += fmt::format ("Effective Scale: {:.2f}\n", lastDisplayInfo.getEffectiveScale ());
  oC += fmt::format ("Is Touch Device: {}\n", isTouchDevice () ? "YES" : "NO");
  oC += fmt::format ("Is Mobile Device: {}\n", isMobileDevice () ? "YES" : "NO");
  oC += fmt::format ("{:.3f} ms/frame ({:.1f} FPS)", 1000.0f / ImGui::GetIO ().Framerate,
                     ImGui::GetIO ().Framerate);

  return oC;
}
