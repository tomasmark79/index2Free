#include "EmscriptenPlatform.hpp"

#include "Logger/Logger.hpp"

#include <Shaders/dyinguniverse/vertex_def.hpp>
#include <Shaders/dyinguniverse/fragment_def.hpp>

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
  createSDL2Window ("Default SDL2 Window", windowWidth_, windowHeight_);
  createOpenGLContext (1);
  initializeGLEW ();
  setupQuad ();
  setupShaders ();
  initializeImGui ();
  ImGuiStyle& style = ImGui::GetStyle ();
  setupImGuiStyle (style);
  updateWindowSize ();
  scaleImGui (this->userConfigurableScale_);
  initInputHandlerCallbacks ();
  // Start the Emscripten main loop
  emscripten_set_main_loop_arg (
      [] (void* userData) {
        EmscriptenPlatform* platform = static_cast<EmscriptenPlatform*> (userData);
        // platform->frameStep ();
        platform->mainLoop ();
      },
      this, 0, 1);
}

void EmscriptenPlatform::shutdown () {
  if (window_) {
    SDL_DestroyWindow (window_);
    window_ = nullptr;
  }
  if (glContext_) {
    SDL_GL_DeleteContext (glContext_);
    glContext_ = nullptr;
  }
  if (imguiContext_) {
    ImGui::DestroyContext (imguiContext_);
    imguiContext_ = nullptr;
  }
}

void EmscriptenPlatform::createSDL2Window (const char* title, int width, int height) {

  // IME hint not typically needed in web browsers

  if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    handleSDLError ("Failed to initialize SDL");
    return;
  }

  // Needed before creating the window
  SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);

  // For Emscripten, we don't need HIGHDPI flag as it's handled differently
  SDL_WindowFlags windowFlags
      = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  window_ = SDL_CreateWindow (title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
                              height, windowFlags);
  if (!window_) {
    handleSDLError ("Failed to create window");
  }
  windowWidth_ = width;
  windowHeight_ = height;
}

void EmscriptenPlatform::setupShaders () {
// Use OpenGL ES shaders for Emscripten
#if defined(IMGUI_IMPL_OPENGL_ES3)
  // WebGL 2.0 / OpenGL ES 3.0
  GLuint vertexShader = compileShader (vertexShader300, GL_VERTEX_SHADER);
  GLuint fragmentShader = compileShader (fragmentShader300, GL_FRAGMENT_SHADER);
#else
  // WebGL 1.0 / OpenGL ES 2.0
  GLuint vertexShader = compileShader (vertexShader200, GL_VERTEX_SHADER);
  GLuint fragmentShader = compileShader (fragmentShader200, GL_FRAGMENT_SHADER);
#endif

  if (vertexShader == 0) {
    handleError ("Failed to compile vertex shader");
    return;
  }

  if (fragmentShader == 0) {
    handleError ("Failed to compile fragment shader");
    glDeleteShader (vertexShader);
    return;
  }

  // Create shader program
  shaderProgram_ = glCreateProgram ();
  glAttachShader (shaderProgram_, vertexShader);
  glAttachShader (shaderProgram_, fragmentShader);
  glLinkProgram (shaderProgram_);

  // Check for linking errors
  GLint success;
  glGetProgramiv (shaderProgram_, GL_LINK_STATUS, &success);
  if (!success) {
    handleError ("Failed to link shader program");
    glDeleteShader (vertexShader);
    glDeleteShader (fragmentShader);
    return;
  }

  // Clean up shaders after linking
  glDeleteShader (vertexShader);
  glDeleteShader (fragmentShader);
}

void EmscriptenPlatform::renderBackground (float deltaTime) {
  // float width = (float)windowWidth_;
  // float height = (float)windowHeight_;
  // glViewport (0, 0, width, height);
  GLboolean depthTestEnabled;
  glGetBooleanv (GL_DEPTH_TEST, &depthTestEnabled);
  glDisable (GL_DEPTH_TEST);

  glUseProgram (shaderProgram_);

#if defined(IMGUI_IMPL_OPENGL_ES3) \
    || (!defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3))
  // OpenGL ES 3.0+ or desktop OpenGL - VAO is available
  glBindVertexArray (vao_);
#else
  // OpenGL ES 2.0 - VAO not available, bind buffers manually
  glBindBuffer (GL_ARRAY_BUFFER, vbo_);
  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glEnableVertexAttribArray (0);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof (float), (void*)0);
#endif

  GLint iResolutionLoc = glGetUniformLocation (shaderProgram_, "iResolution");
  GLint iTimeLoc = glGetUniformLocation (shaderProgram_, "iTime");
  GLint iTimeDeltaLoc = glGetUniformLocation (shaderProgram_, "iTimeDelta");
  GLint iFrameLoc = glGetUniformLocation (shaderProgram_, "iFrame");
  GLint iMouseLoc = glGetUniformLocation (shaderProgram_, "iMouse");

  if (iResolutionLoc != -1)
    glUniform2f (iResolutionLoc, (float)windowWidth_, (float)windowHeight_);
  if (iTimeLoc != -1)
    glUniform1f (iTimeLoc, deltaTime);
  if (iTimeDeltaLoc != -1)
    glUniform1f (iTimeDeltaLoc, 0.016f);
  if (iFrameLoc != -1)
    glUniform1i (iFrameLoc, (int)(deltaTime * 60.0f));
  if (iMouseLoc != -1)
    glUniform4f (iMouseLoc, 0.0f, 0.0f, 0.0f, 0.0f);

  glDrawElements (GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

#if defined(IMGUI_IMPL_OPENGL_ES2)
  // OpenGL ES 2.0 - cleanup manually bound attributes
  glDisableVertexAttribArray (0);
#endif

  if (depthTestEnabled) {
    glEnable (GL_DEPTH_TEST);
  }
}

void EmscriptenPlatform::initializeImGui () {
  IMGUI_CHECKVERSION ();
  ImGui::CreateContext ();
  io_ = &ImGui::GetIO ();
  io_->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io_->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  ImGui_ImplSDL2_InitForOpenGL (window_, glContext_);
  ImGui_ImplOpenGL3_Init (glsl_version_);
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