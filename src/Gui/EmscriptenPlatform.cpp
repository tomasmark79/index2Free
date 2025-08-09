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

// JavaScript function to detect WebGL version
EM_JS (int, detectWebGLVersionJS, (), {
  // Create a temporary canvas for testing
  var canvas = document.createElement('canvas');
  
  // Try WebGL2 first
  var gl2 = canvas.getContext('webgl2');
  if (gl2) {
    console.log('🟢 WebGL 2.0 detected and available');
    console.log('WebGL 2.0 Version:', gl2.getParameter(gl2.VERSION));
    console.log('WebGL 2.0 Renderer:', gl2.getParameter(gl2.RENDERER));
    console.log('WebGL 2.0 Vendor:', gl2.getParameter(gl2.VENDOR));
    console.log('WebGL 2.0 GLSL Version:', gl2.getParameter(gl2.SHADING_LANGUAGE_VERSION));
    return 2;
  }
  
  // Fall back to WebGL1
  var gl1 = canvas.getContext('webgl') || canvas.getContext('experimental-webgl');
  if (gl1) {
    console.log('🟡 WebGL 1.0 detected (WebGL 2.0 not available)');
    console.log('WebGL 1.0 Version:', gl1.getParameter(gl1.VERSION));
    console.log('WebGL 1.0 Renderer:', gl1.getParameter(gl1.RENDERER));
    console.log('WebGL 1.0 Vendor:', gl1.getParameter(gl1.VENDOR));
    console.log('WebGL 1.0 GLSL Version:', gl1.getParameter(gl1.SHADING_LANGUAGE_VERSION));
    return 1;
  }
  
  console.error('🔴 No WebGL support detected!');
  return 0;
});

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

WebGLVersion EmscriptenPlatform::detectWebGLVersion() {
  int version = detectWebGLVersionJS();
  switch (version) {
    case 2:
      return WebGLVersion::WEBGL2;
    case 1:
      return WebGLVersion::WEBGL1;
    default:
      LOG_E_STREAM << "No WebGL support detected! Falling back to WebGL1." << std::endl;
      return WebGLVersion::WEBGL1;
  }
}

void EmscriptenPlatform::logWebGLInfo(WebGLVersion version) {
  switch (version) {
    case WebGLVersion::WEBGL2:
      LOG_I_STREAM << "🚀 Using WebGL 2.0 (OpenGL ES 3.0)" << std::endl;
      break;
    case WebGLVersion::WEBGL1:
      LOG_I_STREAM << "📟 Using WebGL 1.0 (OpenGL ES 2.0)" << std::endl;
      break;
  }
}

void EmscriptenPlatform::decideOpenGLVersionForEmscripten() {
  if (currentWebGLVersion_ == WebGLVersion::WEBGL2) {
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    glsl_version_ = "#version 300 es";
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
  } else {
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    glsl_version_ = "#version 100";
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
  }
}

void EmscriptenPlatform::initialize () {
  // Detect WebGL version before creating context
  currentWebGLVersion_ = detectWebGLVersion();
  logWebGLInfo(currentWebGLVersion_);
  
  const bool isInfiniteLoop = true; // Emscripten main loop runs indefinitely
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
      this, 0, isInfiniteLoop);
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
