#include "EmscriptenPlatform.hpp"

#include "Logger/Logger.hpp"
#include "../Shaders/ShaderConvertor.hpp"

#include <emscripten.h>
#include <emscripten/html5.h>

namespace CustomJS {

  EM_JS (float, getDevicePixelRatioJS, (), { return window.devicePixelRatio || 1.0; });

  EM_JS (int, getScreenWidthJS, (), { return window.screen.width; });

  EM_JS (int, getScreenHeightJS, (), { return window.screen.height; });

  EM_JS (int, getWindowWidthJS, (), { return window.innerWidth; });

  EM_JS (int, getWindowHeightJS, (), { return window.innerHeight; });

  EM_JS (int, isTouchDeviceJS, (),
         { return ('ontouchstart' in window || navigator.maxTouchPoints > 0) ? 1 : 0; });

  EM_JS (int, isDocumentFullscreenJS, (), {
    return (document.fullscreenElement || document.webkitFullscreenElement
            || document.mozFullScreenElement)
               ? 1
               : 0;
  });

  EM_JS (int, detectWebGLVersionJS, (), {
    var canvas = document.createElement ('canvas');
    var gl = canvas.getContext ('webgl2');
    if (gl)
      return 2;
    gl = canvas.getContext ('webgl') || canvas.getContext ('experimental-webgl');
    return gl ? 1 : 0;
  });

  EM_JS (void, getDisplayInfoJS, (float* dpr, int* width, int* height), {
    setValue (dpr, window.devicePixelRatio || 1.0, 'float');
    setValue (width, window.innerWidth, 'i32');
    setValue (height, window.innerHeight, 'i32');
  });

  // clang-format off
EM_JS(bool, isMobileDeviceJS, (), {
  return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
});
  // clang-format on

} // namespace CustomJS

namespace EmscriptenDisplayInfo {

  bool DisplayInfo::isScaled () const {
    return devicePixelRatio > 1.0f;
  }

  float DisplayInfo::getEffectiveScale () const {
    return isScaled () ? devicePixelRatio : 1.0f;
  }

  void DisplayInfo::update () {
    CustomJS::getDisplayInfoJS (&devicePixelRatio, &windowWidth, &windowHeight);
  }
}

WebGLVersion EmscriptenPlatform::detectWebGLVersionByJS () {
  int version = CustomJS::detectWebGLVersionJS ();
  switch (version) {
  case 2:
    LOG_I_STREAM << "🚀 Using WebGL 2.0 (OpenGL ES 3.0)" << std::endl;
    return WebGLVersion::WEBGL2;

  case 1:
    LOG_I_STREAM << "📟 Using WebGL 1.0 (OpenGL ES 2.0)" << std::endl;
    return WebGLVersion::WEBGL1;
  default:
    LOG_E_STREAM << "No WebGL support detected! Falling back to WebGL1." << std::endl;
    return WebGLVersion::WEBGL1;
  }
}

void EmscriptenPlatform::decideOpenGLVersionForEmscripten () {
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

void EmscriptenPlatform::updateWindowSize () {
  int width, height;
  SDL_GL_GetDrawableSize (window_, &width, &height);
  if (width != windowWidth_ || height != windowHeight_) {
    windowWidth_ = width;
    windowHeight_ = height;
    devicePixelRatio_ = CustomJS::getDevicePixelRatioJS ();
    io_->DisplaySize = ImVec2 ((float)windowWidth_, (float)windowHeight_);
    io_->DisplayFramebufferScale = ImVec2 (devicePixelRatio_, devicePixelRatio_);
  }
}

void EmscriptenPlatform::initialize () {
  currentWebGLVersion_ = detectWebGLVersionByJS ();
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

int EmscriptenPlatform::getShaderTarget () {
  if (currentWebGLVersion_ == WebGLVersion::WEBGL2) {
    return static_cast<int> (ShaderTarget::WebGL2);
  } else {
    return static_cast<int> (ShaderTarget::WebGL1);
  }
}

void EmscriptenPlatform::mainLoop () {
  // For Emscripten: Single iteration - called by emscripten_set_main_loop_arg
  this->updateWindowSize ();

  SDL_Event event;
  while (SDL_PollEvent (&event)) {
    ImGui_ImplSDL2_ProcessEvent (&event);
    bool done = inputHandler.processEvent (event); // own event processing
    if (done) {
      emscripten_cancel_main_loop (); // Stop the main loop in Emscripten
      return;
    }
  }

  ImGui_ImplOpenGL3_NewFrame ();
  ImGui_ImplSDL2_NewFrame ();
  ImGui::NewFrame ();

  this->buildImguiContent ();

  ImGui::Render ();
  glViewport (0, 0, windowWidth_, windowHeight_);
  glClearColor (0.45f, 0.55f, 0.60f, 1.00f);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Render background shader - cumulative time using SDL_GetTicks for better precision
  static float totalTime = 0.0f;
  static Uint32 lastTime = SDL_GetTicks ();
  Uint32 currentTime = SDL_GetTicks ();

  float deltaTime = (currentTime - lastTime) / 1000.0f;
  totalTime += deltaTime;
  lastTime = currentTime;

  renderBackground (totalTime); // Pass cumulative time, not delta
  ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());
  SDL_GL_SwapWindow (window_);
}
