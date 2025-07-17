#include "DesktopPlatform.hpp"

#include <Shaders/dyinguniverse/vertex_def.hpp>
#include <Shaders/dyinguniverse/fragment_def.hpp>

void DesktopPlatform::initialize () {
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
  mainLoop ();
}

void DesktopPlatform::shutdown () {
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

void DesktopPlatform::createSDL2Window (const char* title, int width, int height) {

// Enable IME UI on desktop platforms
#ifdef SDL_HINT_IME_SHOW_UI
  SDL_SetHint (SDL_HINT_IME_SHOW_UI, "1");
#endif

  if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    handleSDLError ("Failed to initialize SDL");
    return;
  }

  // Needed before creating the window
  SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);

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

void DesktopPlatform::initializeGLEW () {

#ifndef IMGUI_IMPL_OPENGL_ES2
  if (glewInit () != GLEW_OK) {
    printf ("Error initializing GLEW\n");
    return;
  }
#endif
}

void DesktopPlatform::setupShaders () {
  // Compile vertex shader
  GLuint vertexShader = compileShader (vertexShader330, GL_VERTEX_SHADER);
  if (vertexShader == 0) {
    handleError ("Failed to compile vertex shader");
    return;
  }

  // Compile fragment shader
  GLuint fragmentShader = compileShader (fragmentShader330, GL_FRAGMENT_SHADER);
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

void DesktopPlatform::renderBackground (float deltaTime) {
  // float width = (float)windowWidth_;
  // float height = (float)windowHeight_;
  // glViewport (0, 0, width, height);
  GLboolean depthTestEnabled;
  glGetBooleanv (GL_DEPTH_TEST, &depthTestEnabled);
  glDisable (GL_DEPTH_TEST);

  glUseProgram (shaderProgram_);
  glBindVertexArray (vao_);

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

  if (depthTestEnabled) {
    glEnable (GL_DEPTH_TEST);
  }
}

void DesktopPlatform::initializeImGui () {
  IMGUI_CHECKVERSION ();
  ImGui::CreateContext ();
  io_ = &ImGui::GetIO ();
  io_->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io_->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  ImGui_ImplSDL2_InitForOpenGL (window_, glContext_);
  ImGui_ImplOpenGL3_Init (glsl_version_);
}

std::string DesktopPlatform::getOverlayContent () {
  std::string oC = "";
  oC += ("=== Overlay Window ===\n");
  oC += ("Window Size: " + std::to_string (windowWidth_) + "x" + std::to_string (windowHeight_)
         + "\n");
  oC += ("ImGui Display Size: " + std::to_string (io_->DisplaySize.x) + " x "
         + std::to_string (io_->DisplaySize.y) + "\n");
  oC += ("User Configurable Scale: " + std::to_string (userConfigurableScale_) + "\n");
  oC += ("Device Pixel Ratio: " + std::to_string (devicePixelRatio_) + "\n");
  oC += ("Base Font Size: " + std::to_string (BASE_FONT_SIZE) + "\n");
  oC += ("ImGui Display Framebuffer Scale: " + std::to_string (io_->DisplayFramebufferScale.x)
         + " x " + std::to_string (io_->DisplayFramebufferScale.y) + "\n");
  oC += ("=== Desktop Display Info ===\n");
  oC += ("SDL Window ID: " + std::to_string (SDL_GetWindowID (window_)) + "\n");
  oC += ("SDL Window Display Index: " + std::to_string (SDL_GetWindowDisplayIndex (window_))
         + "\n");
  int displayIndex = SDL_GetWindowDisplayIndex (window_);
  float ddpi, hdpi, vdpi;
  if (SDL_GetDisplayDPI (displayIndex, &ddpi, &hdpi, &vdpi) == 0) {
    oC += ("Display DPI: " + std::to_string (ddpi) + " (DDPI), " + std::to_string (hdpi)
           + " (HDPI), " + std::to_string (vdpi) + " (VDPI)\n");
    devicePixelRatio_ = ddpi / 96.0f; // 96 DPI is the standard baseline
  } else {
    oC += ("Failed to get display DPI\n");
    devicePixelRatio_ = 1.0f; // Fallback to 1.0 if DPI detection fails
  }
  oC += fmt::format ("{:.3f} ms/frame ({:.1f} FPS)", 1000.0f / ImGui::GetIO ().Framerate,
                     ImGui::GetIO ().Framerate);
  return oC;
}

void DesktopPlatform::updateWindowSize () {
  int width, height;
  SDL_GL_GetDrawableSize (window_, &width, &height);
  if (width != windowWidth_ || height != windowHeight_) {
    windowWidth_ = width;
    windowHeight_ = height;
    // Desktop platforms - use SDL to get DPI pixel ratio
    float ddpi, hdpi, vdpi;
    int displayIndex = SDL_GetWindowDisplayIndex (window_);
    if (SDL_GetDisplayDPI (displayIndex, &ddpi, &hdpi, &vdpi) == 0) {
      devicePixelRatio_ = ddpi / 96.0f; // 96 DPI is the standard baseline
    } else {
      devicePixelRatio_ = 1.0f; // Fallback to 1.0 if DPI detection fails
    }
    io_->DisplaySize = ImVec2 ((float)windowWidth_, (float)windowHeight_);
    io_->DisplayFramebufferScale = ImVec2 (devicePixelRatio_, devicePixelRatio_);
  }
}
