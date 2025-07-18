#include "DesktopPlatform.hpp"

void DesktopPlatform::initialize () {
  createSDL2Window ("Desktop SDL2 Window", windowWidth_, windowHeight_);
  createOpenGLContext (1);
  setupQuad ();
  setupShaders ();
  initializeImGui ();
  updateWindowSize ();
  scaleImGui (this->userScaleFactor);
  initInputHandlerCallbacks ();
  mainLoop ();
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
