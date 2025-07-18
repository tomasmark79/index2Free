#include "DesktopPlatform.hpp"

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

void DesktopPlatform::initializeGLEW () {

#ifndef IMGUI_IMPL_OPENGL_ES2
  if (glewInit () != GLEW_OK) {
    printf ("Error initializing GLEW\n");
    return;
  }
#endif
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
