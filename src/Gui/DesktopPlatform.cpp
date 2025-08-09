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

void DesktopPlatform::mainLoop () {
  // For Desktop: Traditional game loop
  bool done = false;
  SDL_Event event;

  while (!done) {
    this->updateWindowSize ();

    while (SDL_PollEvent (&event)) {
      ImGui_ImplSDL2_ProcessEvent (&event);
      done = inputHandler.processEvent (event); // own event processing
    }

    ImGui_ImplOpenGL3_NewFrame ();
    ImGui_ImplSDL2_NewFrame ();
    ImGui::NewFrame ();

    this->buildImguiContent ();

    ImGui::Render ();
    glViewport (0, 0, windowWidth_, windowHeight_);
    glClearColor (0.45f, 0.55f, 0.60f, 1.00f);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render background shader - cumulative time
    static float totalTime = 0.0f;
    static Uint32 lastTime = SDL_GetTicks ();
    Uint32 currentTime = SDL_GetTicks ();

    float deltaTime = (currentTime - lastTime) / 1000.0f;
    totalTime += deltaTime;
    lastTime = currentTime;

    renderBackground (totalTime); // Pass cumulative time, not delta
    ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());
    SDL_GL_SwapWindow (window_);

    // Frame rate limiting for desktop
    static const int targetFramerate = 30;
    static const int frameDelay = 1000 / targetFramerate;
    static Uint32 lastFrameTime = SDL_GetTicks ();

    Uint32 currentFrameTime = SDL_GetTicks ();
    if (currentFrameTime - lastFrameTime < frameDelay) {
      SDL_Delay (frameDelay - (currentFrameTime - lastFrameTime));
    }
    lastFrameTime = SDL_GetTicks ();
  }
}