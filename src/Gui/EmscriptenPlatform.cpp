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
  scaleImGui (this->userScaleFactor);
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
