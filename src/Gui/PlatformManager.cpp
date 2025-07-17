#include "PlatformManager.hpp"
#include <memory>

#include "GuiStrings.hpp"

// Static platform instance
#if defined(__EMSCRIPTEN__)
  #include "EmscriptenPlatform.hpp"
static std::unique_ptr<EmscriptenPlatform> gPlatform = nullptr;
#else
  #include "DesktopPlatform.hpp"
static std::unique_ptr<DesktopPlatform> gPlatform = nullptr;
#endif

// Function to initialize the platform
void initializePlatform () {
#if defined(__EMSCRIPTEN__)
  gPlatform = std::make_unique<EmscriptenPlatform> ();
  gPlatform->initialize ();
#else
  gPlatform = std::make_unique<DesktopPlatform> ();
  gPlatform->initialize ();
#endif
}

void PlatformManager::createOpenGLContext (int swapInterval) {
  decideOpenGLVersion ();
  glContext_ = SDL_GL_CreateContext (window_);
  if (!glContext_) {
    handleSDLError ("Failed to create OpenGL context");
  }
  SDL_GL_MakeCurrent (window_, glContext_);
  SDL_GL_SetSwapInterval (swapInterval); // Set vsync to the specified FPS
}

GLuint PlatformManager::compileShader (const char* shaderSource, GLenum shaderType) {
  GLuint shader = glCreateShader (shaderType);
  if (shader == 0) {
    handleGLError ("Failed to create shader");
    return 0;
  }

  glShaderSource (shader, 1, &shaderSource, nullptr);
  glCompileShader (shader);

  // Check for compilation errors
  GLint success;
  glGetShaderiv (shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLint logLength;
    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> infoLog (logLength);
    glGetShaderInfoLog (shader, logLength, nullptr, infoLog.data ());
    handleError ("Failed to compile shader", infoLog.data ());
    glDeleteShader (shader);
    return 0;
  }

  return shader;
}

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

void PlatformManager::setupQuad () {

  float vertices[] = {
    // positions (x, y) - v rozmezí -1 až 1
    -1.0f, -1.0f, // bottom left
    1.0f,  -1.0f, // bottom right
    1.0f,  1.0f,  // top right
    -1.0f, 1.0f   // top left
  };

  unsigned int indices[] = {
    0, 1, 2, // first triangle
    2, 3, 0  // second triangle
  };

#if defined(IMGUI_IMPL_OPENGL_ES3) \
    || (!defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3))
  // OpenGL ES 3.0+ or desktop OpenGL - VAO is available
  glGenVertexArrays (1, &vao_);
  glBindVertexArray (vao_);
#endif

  glGenBuffers (1, &vbo_);
  glGenBuffers (1, &ebo_);

  glBindBuffer (GL_ARRAY_BUFFER, vbo_);
  glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);

  // Vertex attribute (location = 0, 2 floats per vertex)
  glEnableVertexAttribArray (0);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof (float), (void*)0);

#if defined(IMGUI_IMPL_OPENGL_ES3) \
    || (!defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3))
  // OpenGL ES 3.0+ or desktop OpenGL - unbind VAO
  glBindVertexArray (0);
#else
  // OpenGL ES 2.0 - disable vertex attributes for now
  glDisableVertexAttribArray (0);
#endif
}

void PlatformManager::setupImGuiStyle (ImGuiStyle& style) {
  style.WindowRounding = 8.0f; // Větší zaoblení pro futuristický look
  style.FrameRounding = 6.0f;
  style.GrabRounding = 6.0f;
  style.ScrollbarRounding = 6.0f;
  style.FrameBorderSize = 1.5f; // Tlustší okraje pro neonový efekt
  style.WindowBorderSize = 2.0f;
  style.WindowPadding = ImVec2 (12.0f, 12.0f); // Větší padding
  style.FramePadding = ImVec2 (8.0f, 4.0f);

  // Tmavý futuristický theme - MODRÁ PALETA
  style.Colors[ImGuiCol_WindowBg]
      = ImVec4 (0.05f, 0.08f, 0.15f, 0.85f); // Tmavě modrá s průhledností
  style.Colors[ImGuiCol_ChildBg] = ImVec4 (0.03f, 0.06f, 0.12f, 0.90f); // Ještě tmavší modrá
  style.Colors[ImGuiCol_PopupBg] = ImVec4 (0.08f, 0.12f, 0.20f, 0.95f); // Popup pozadí - modrá

  // Neonové okraje - různé odstíny modré
  style.Colors[ImGuiCol_Border] = ImVec4 (0.20f, 0.60f, 1.00f, 0.80f);       // Jasná modrá okraj
  style.Colors[ImGuiCol_BorderShadow] = ImVec4 (0.10f, 0.30f, 0.60f, 0.50f); // Tmavší modrý stín

  // Frame elementy - tmavé s modrými akcenty
  style.Colors[ImGuiCol_FrameBg] = ImVec4 (0.08f, 0.12f, 0.20f, 0.80f);
  style.Colors[ImGuiCol_FrameBgHovered]
      = ImVec4 (0.15f, 0.40f, 0.70f, 0.50f);                                  // Střední modrá hover
  style.Colors[ImGuiCol_FrameBgActive] = ImVec4 (0.30f, 0.60f, 1.00f, 0.70f); // Jasná modrá active

  // Title bar - modré odstíny
  style.Colors[ImGuiCol_TitleBg] = ImVec4 (0.10f, 0.25f, 0.50f, 0.90f);
  style.Colors[ImGuiCol_TitleBgActive] = ImVec4 (0.20f, 0.45f, 0.80f, 0.95f);
  style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4 (0.05f, 0.15f, 0.35f, 0.80f);

  // Menu bar
  style.Colors[ImGuiCol_MenuBarBg] = ImVec4 (0.05f, 0.10f, 0.18f, 0.90f);

  // Scrollbar - modré odstíny
  style.Colors[ImGuiCol_ScrollbarBg] = ImVec4 (0.03f, 0.06f, 0.12f, 0.70f);
  style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4 (0.20f, 0.50f, 0.80f, 0.60f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4 (0.30f, 0.60f, 0.90f, 0.80f);
  style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4 (0.40f, 0.70f, 1.00f, 1.00f);

  // Checkmark - světle modrá
  style.Colors[ImGuiCol_CheckMark] = ImVec4 (0.40f, 0.80f, 1.00f, 1.00f);

  // Slider - modré odstíny
  style.Colors[ImGuiCol_SliderGrab] = ImVec4 (0.30f, 0.60f, 0.90f, 0.80f);
  style.Colors[ImGuiCol_SliderGrabActive] = ImVec4 (0.50f, 0.80f, 1.00f, 1.00f);

  // Buttons - modré odstíny
  style.Colors[ImGuiCol_Button] = ImVec4 (0.10f, 0.20f, 0.35f, 0.80f);
  style.Colors[ImGuiCol_ButtonHovered] = ImVec4 (0.20f, 0.45f, 0.75f, 0.80f); // Střední modrá hover
  style.Colors[ImGuiCol_ButtonActive] = ImVec4 (0.35f, 0.65f, 1.00f, 0.90f);  // Jasná modrá active

  // Headers - světlejší modré odstíny
  style.Colors[ImGuiCol_Header] = ImVec4 (0.15f, 0.35f, 0.65f, 0.70f);
  style.Colors[ImGuiCol_HeaderHovered] = ImVec4 (0.25f, 0.50f, 0.80f, 0.80f);
  style.Colors[ImGuiCol_HeaderActive] = ImVec4 (0.40f, 0.70f, 1.00f, 0.90f);

  // Separators - modré
  style.Colors[ImGuiCol_Separator] = ImVec4 (0.20f, 0.45f, 0.75f, 0.60f);
  style.Colors[ImGuiCol_SeparatorHovered] = ImVec4 (0.30f, 0.60f, 0.90f, 0.80f);
  style.Colors[ImGuiCol_SeparatorActive] = ImVec4 (0.45f, 0.75f, 1.00f, 1.00f);

  // Text - bílý s lehkým modrým nádechem
  style.Colors[ImGuiCol_Text]
      = ImVec4 (0.95f, 0.98f, 1.00f, 1.00f); // Bílý text s lehkým modrým nádechem
  style.Colors[ImGuiCol_TextDisabled]
      = ImVec4 (0.40f, 0.50f, 0.70f, 0.80f); // Šedo-modrý disabled text

  // Textové odkazy - jasně modré pro tmavé pozadí
  style.Colors[ImGuiCol_TextLink] = ImVec4 (0.60f, 0.85f, 1.00f, 1.00f); // Světle modrá pro odkazy

  // Resize grip - světle modrá
  style.Colors[ImGuiCol_ResizeGrip] = ImVec4 (0.25f, 0.55f, 0.85f, 0.60f);
  style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4 (0.35f, 0.65f, 0.95f, 0.80f);
  style.Colors[ImGuiCol_ResizeGripActive] = ImVec4 (0.50f, 0.80f, 1.00f, 1.00f);

  // Tab colors - modré odstíny
  style.Colors[ImGuiCol_Tab] = ImVec4 (0.10f, 0.25f, 0.45f, 0.70f);
  style.Colors[ImGuiCol_TabHovered] = ImVec4 (0.25f, 0.50f, 0.80f, 0.80f);
  style.Colors[ImGuiCol_TabActive] = ImVec4 (0.35f, 0.65f, 0.95f, 0.90f);
  style.Colors[ImGuiCol_TabUnfocused] = ImVec4 (0.05f, 0.15f, 0.30f, 0.60f);
  style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4 (0.15f, 0.35f, 0.60f, 0.70f);

  // Plot colors - modré odstíny
  style.Colors[ImGuiCol_PlotLines] = ImVec4 (0.30f, 0.70f, 1.00f, 1.00f); // Světle modrá
  style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4 (0.50f, 0.80f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogram] = ImVec4 (0.20f, 0.60f, 0.90f, 1.00f); // Střední modrá
  style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4 (0.40f, 0.75f, 1.00f, 1.00f);

  // Table colors - modré odstíny
  style.Colors[ImGuiCol_TableHeaderBg] = ImVec4 (0.10f, 0.30f, 0.55f, 0.80f);
  style.Colors[ImGuiCol_TableBorderStrong] = ImVec4 (0.20f, 0.50f, 0.80f, 0.80f);
  style.Colors[ImGuiCol_TableBorderLight] = ImVec4 (0.15f, 0.35f, 0.60f, 0.50f);
  style.Colors[ImGuiCol_TableRowBg] = ImVec4 (0.00f, 0.00f, 0.00f, 0.00f);
  style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4 (0.05f, 0.10f, 0.20f, 0.30f);

  // Drag and drop - světle modrá
  style.Colors[ImGuiCol_DragDropTarget] = ImVec4 (0.40f, 0.75f, 1.00f, 0.80f);

  // Navigation - modré odstíny
  style.Colors[ImGuiCol_NavHighlight] = ImVec4 (0.30f, 0.65f, 1.00f, 0.80f);
  style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4 (0.80f, 0.90f, 1.00f, 0.70f);
  style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4 (0.10f, 0.20f, 0.40f, 0.20f);

  // Modal window dim - tmavě modrá
  style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4 (0.02f, 0.05f, 0.12f, 0.60f);

  // Uložit výchozí styl pro pozdější použití při škálování
  style_ = &style;
}

void PlatformManager::initInputHandlerCallbacks () {
  inputHandler.setScaleCallback ([this] (float delta) {
    // userConfigurableScale_ += delta;
    // userConfigurableScale_ = std::max (0.1f, userConfigurableScale_);
    // requiredScaleChange_ = true;
  });
  inputHandler.setActionCallback (InputAction::ToggleFullscreen, [this] () { /*toggleFullscreen ()*/
                                                                             ;
  });

  inputHandler.setActionCallback (InputAction::VolumeUp, [&/*audio*/] () mutable {
    // currVol = std::min (100, currVol + 5);
    // audio.setVolume (currVol);
    // LOG_I_STREAM << "Volume increased to: " << currVol << std::endl;
  });
  inputHandler.setActionCallback (InputAction::VolumeDown, [&/*audio*/] () mutable {
    // currVol = std::max (0, currVol - 5);
    // audio.setVolume (currVol);
    // LOG_D_STREAM << "Volume decreased to: " << currVol << std::endl;
  });
  inputHandler.setActionCallback (InputAction::Mute, [&/*audio*/] () {
    // if (audio.isPlaying ()) {
    //   audio.pauseMusic ();
    //   LOG_D_STREAM << "Music paused" << std::endl;
    // } else {
    //   audio.resumeMusic ();
    //   LOG_D_STREAM << "Music resumed" << std::endl;
    // }
  });
}

void PlatformManager::mainLoop () {
  bool done = false;

  SDL_Event event;
  while (!done) {
    this->updateWindowSize ();

    while (SDL_PollEvent (&event)) {
      ImGui_ImplSDL2_ProcessEvent (&event);
      done = inputHandler.processEvent (event); // own event processing
      if (done) {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop (); // Stop the main loop in Emscripten
#else
        done = true; // Stop the main loop on desktop platforms
#endif
      }
    }

    ImGui_ImplOpenGL3_NewFrame ();
    ImGui_ImplSDL2_NewFrame ();
    ImGui::NewFrame ();

    bool showDemo = true;
    if (showDemo)
      ImGui::ShowDemoWindow (&showDemo);

    bool showOverlay = true;
    if (showOverlay) {
      printOverlayWindow ();
    }

    // Render GUI
    ImGui::Render ();
    glViewport (0, 0, windowWidth_, windowHeight_);
    glClearColor (0.45f, 0.55f, 0.60f, 1.00f);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render background shader
    float time = SDL_GetTicks () / 1000.0f;
    renderBackground (time);

    ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());
    SDL_GL_SwapWindow (window_);
  }
}

void PlatformManager::scaleImGui (int userScaleFactor) {
  ImGuiStyle defaultStyle_;
  float scalingFactor = userScaleFactor * devicePixelRatio_;
  float fontSize = BASE_FONT_SIZE * scalingFactor;
  fontSize = std::max (1.0f, roundf (fontSize));
  ImFontConfig fontCfg = {};
  fontCfg.RasterizerDensity = scalingFactor;
  static const ImWchar czRanges[]
      = { 0x0020, 0x00FF, 0x0100, 0x017F, 0x0200, 0x024F, 0x0401, 0x045F, 0x0402, 0x045F,
          0x0403, 0x045F, 0x0404, 0x045F, 0x0405, 0x045F, 0x0406, 0x045F, 0x0407, 0x045F,
          0x0408, 0x045F, 0x0409, 0x045F, 0x040A, 0x045F, 0x040B, 0x045F, 0x040C, 0x045F,
          0x040D, 0x045F, 0x040E, 0x045F, 0x040F, 0x045F, 0 };
  std::filesystem::path fnt = AssetContext::getAssetsPath () / "fonts" / "Comfortaa-Light.otf";
  io_->Fonts->Clear ();
  io_->Fonts->AddFontFromFileTTF (fnt.c_str (), fontSize, &fontCfg, czRanges);
  io_->Fonts->Build ();
  // Reset style to defaults
  ImGui::GetStyle () = defaultStyle_;
  // and then apply scaling
  ImGui::GetStyle ().ScaleAllSizes (scalingFactor);
}

void PlatformManager::printOverlayWindow () {
  bool showOverlay = true;
  ImGui::PushID ("OverlayWindow");
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize
                                 | ImGuiWindowFlags_NoSavedSettings
                                 | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
  ImGui::Begin ("Overlay", &showOverlay, windowFlags);
  ImGui::Text (this->getOverlayContent ().c_str ());
  ImGui::End ();
  ImGui::PopID ();
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
