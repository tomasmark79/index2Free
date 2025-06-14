#include "PlatformManager.hpp"

void initializePlatform () {
#if defined(__EMSCRIPTEN__)
  static EmscriptenPlatform platform;
  platform.initialize ();
#else
  static DesktopPlatform platform;
  platform.initialize ();
#endif
}
void shutdownPlatform () {
#if defined(__EMSCRIPTEN__)
  static EmscriptenPlatform platform;
  platform.shutdown ();
#else
  static DesktopPlatform platform;
  platform.shutdown ();
#endif
}

void PlatformManager::handleSDLError (const char* message) const {
  const char* error = SDL_GetError ();
  if (error && *error) {
    handleError (message, error);
  } else {
    handleError (message);
  }
}
void PlatformManager::handleGLError (const char* message) const {
  const char* error = reinterpret_cast<const char*> (glGetError ());
  if (error && *error) {
    handleError (message, error);
  } else {
    handleError (message);
  }
}
void PlatformManager::handleImGuiError (const char* message) const {
  handleError (message);
};
void PlatformManager::handleError (const char* message) const {
  LOG_E_FMT ("%s", message);
};
void PlatformManager::handleError (const char* message, const char* error) const {
  if (error && *error) {
    LOG_E_FMT ("%s: %s", message, error);
  } else {
    LOG_E_FMT ("%s", message);
  }
};
void PlatformManager::handleError (const char* message, int errorCode) const {
  LOG_E_FMT ("%s: %d", message, errorCode);
};

void PlatformManager::decideOpenGLVersion () {
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100 (WebGL 1.0)
  glsl_version_ = "#version 100";
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
  glsl_version_ = "#version 300 es";
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
  // GL 3.2 Core + GLSL 150
  glsl_version_ = "#version 150";
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS,
                       SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
  // GL 3.0 + GLSL 130
  glsl_version_ = "#version 130";
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
}

void DesktopPlatform::initialize () {
  createSDL2Window ("Default SDL2 Window", 1920, 1080);
  createOpenGLContext ();
  initializeImGui ();
  runLoop ();
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

#ifdef SDL_HINT_IME_SHOW_UI
  SDL_SetHint (SDL_HINT_IME_SHOW_UI, "1"); // Enable IME UI on desktop platforms
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

void DesktopPlatform::createOpenGLContext () {
  decideOpenGLVersion ();

  glContext_ = SDL_GL_CreateContext (window_);
  if (!glContext_) {
    handleSDLError ("Failed to create OpenGL context");
  }
  SDL_GL_MakeCurrent (window_, glContext_);
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

void PlatformManager::runLoop () {
  bool done = false;
  SDL_Event event;
  while (!done) {
    while (SDL_PollEvent (&event)) {
      ImGui_ImplSDL2_ProcessEvent (&event);
      if (event.type == SDL_QUIT) {
        done = true;
      }
    }

    ImGui_ImplOpenGL3_NewFrame ();
    ImGui_ImplSDL2_NewFrame ();
    ImGui::NewFrame ();

    bool showDemo = true;
    if (showDemo)
      ImGui::ShowDemoWindow (&showDemo);

    // Render your GUI here
    ImGui::Render ();
    glViewport (0, 0, windowWidth_, windowHeight_);
    glClearColor (0.45f, 0.55f, 0.60f, 1.00f);
    glClear (GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());

    SDL_GL_SwapWindow (window_);
  }
}

// void EmscriptenPlatform::initialize () {
//   createWindow ("Emscripten Window", 1920, 1080);
// }

// void EmscriptenPlatform::shutdown () {
//   if (window_) {
//     SDL_DestroyWindow (window_);
//     window_ = nullptr;
//   }
//   if (glContext_) {
//     SDL_GL_DeleteContext (glContext_);
//     glContext_ = nullptr;
//   }
//   if (imguiContext_) {
//     ImGui::DestroyContext (imguiContext_);
//     imguiContext_ = nullptr;
//   }
// }

// void EmscriptenPlatform::createOpenGLContext () {
//   glContext_ = SDL_GL_CreateContext (window_);
//   if (!glContext_) {
//     handleSDLError ("Failed to create OpenGL context");
//   }
//   SDL_GL_MakeCurrent (window_, glContext_);
// }

// void EmscriptenPlatform::createWindow (const char* title, int width, int height) {
//   window_ = SDL_CreateWindow (title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
//                               height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
//   if (!window_) {
//     handleSDLError ("Failed to create window");
//   }
//   windowWidth_ = width;
//   windowHeight_ = height;
// }

// void EmscriptenPlatform::initializeImGui () {
//   IMGUI_CHECKVERSION ();
//   ImGui::CreateContext ();
//   io_ = ImGui::GetIO ();
// }