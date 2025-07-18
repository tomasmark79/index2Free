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

#include <Shaders/dyinguniverse/vertex_def.hpp>
#include <Shaders/dyinguniverse/fragment_def.hpp>
#include <imgui_internal.h>

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

// Function to shut down the platform
void PlatformManager::shutdown () {
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

// Function to create an SDL2 window
void PlatformManager::createSDL2Window (const char* title, int width, int height) {

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

// Create OpenGL context with the specified swap interval
// Set vsync to 1 for vertical sync
// Set swap interval to 0 for no vsync
// Set swap interval to -1 for adaptive vsync
void PlatformManager::createOpenGLContext (int swapInterval) {
  decideOpenGLVersion ();
  glContext_ = SDL_GL_CreateContext (window_);
  if (!glContext_) {
    handleSDLError ("Failed to create OpenGL context");
  }
  SDL_GL_MakeCurrent (window_, glContext_);
  SDL_GL_SetSwapInterval (swapInterval); // Set vsync

  // GLEW initialization only for desktop platforms
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3)
  if (glewInit () != GLEW_OK) {
    handleGLError ("Error initializing GLEW");
    return;
  }
#endif
}

// Setup shaders based on the OpenGL version
void PlatformManager::setupShaders () {
#if defined(IMGUI_IMPL_OPENGL_ES3)
  // WebGL 2.0 / OpenGL ES 3.0
  GLuint vertexShader = compileShader (vertexShader300, GL_VERTEX_SHADER);
  GLuint fragmentShader = compileShader (fragmentShader300, GL_FRAGMENT_SHADER);
#elif defined(IMGUI_IMPL_OPENGL_ES2)
  // WebGL 1.0 / OpenGL ES 2.0
  GLuint vertexShader = compileShader (vertexShader200, GL_VERTEX_SHADER);
  GLuint fragmentShader = compileShader (fragmentShader200, GL_FRAGMENT_SHADER);
#else
  // Desktop OpenGL
  GLuint vertexShader = compileShader (vertexShader330, GL_VERTEX_SHADER);
  GLuint fragmentShader = compileShader (fragmentShader330, GL_FRAGMENT_SHADER);
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
  }

  // Clean up shaders after linking
  glDeleteShader (vertexShader);
  glDeleteShader (fragmentShader);
}

// Compile shader from source code
// Returns the shader ID or 0 on failure
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

// Decide OpenGL version based on platform and settings
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

// Setup a simple quad for rendering
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

// Initialize ImGui context and settings
void PlatformManager::initializeImGui () {
  IMGUI_CHECKVERSION ();
  ImGui::CreateContext ();
  io_ = &ImGui::GetIO ();
  io_->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io_->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  ImGui_ImplSDL2_InitForOpenGL (window_, glContext_);
  ImGui_ImplOpenGL3_Init (glsl_version_);

  styleBackup_ = &ImGui::GetStyle ();
  applyStyleLila (style_);
  style_ = &ImGui::GetStyle ();
}

// Apply the Lila style to ImGui
void PlatformManager::applyStyleLila (ImGuiStyle* dst) {
  ImGuiStyle* style = dst ? dst : &ImGui::GetStyle ();
  ImVec4* colors = style->Colors;

  style->WindowRounding = 8.0f; // Větší zaoblení pro futuristický look
  style->FrameRounding = 6.0f;
  style->GrabRounding = 6.0f;
  style->ScrollbarRounding = 6.0f;
  style->FrameBorderSize = 1.5f; // Tlustší okraje pro neonový efekt
  style->WindowBorderSize = 2.0f;
  style->WindowPadding = ImVec2 (12.0f, 12.0f); // Větší padding
  style->FramePadding = ImVec2 (8.0f, 4.0f);

  // Tmavý futuristický theme - FIALOVÁ PALETA
  colors[ImGuiCol_Text]
      = ImVec4 (0.95f, 0.90f, 1.00f, 1.00f); // Bílý text s lehkým fialovým nádechem
  colors[ImGuiCol_TextDisabled] = ImVec4 (0.50f, 0.40f, 0.60f, 0.80f); // Šedo-fialový disabled text
  colors[ImGuiCol_WindowBg] = ImVec4 (0.08f, 0.05f, 0.15f, 0.94f);     // Tmavě fialová
  colors[ImGuiCol_ChildBg] = ImVec4 (0.06f, 0.03f, 0.12f, 0.90f);      // Ještě tmavší fialová
  colors[ImGuiCol_PopupBg] = ImVec4 (0.12f, 0.08f, 0.20f, 0.95f);      // Popup pozadí - fialová
  colors[ImGuiCol_Border] = ImVec4 (0.60f, 0.20f, 1.00f, 0.80f);       // Jasná fialová okraj
  colors[ImGuiCol_BorderShadow] = ImVec4 (0.30f, 0.10f, 0.60f, 0.50f); // Tmavší fialový stín
  colors[ImGuiCol_FrameBg] = ImVec4 (0.12f, 0.08f, 0.20f, 0.80f);
  colors[ImGuiCol_FrameBgHovered] = ImVec4 (0.40f, 0.15f, 0.70f, 0.50f); // Střední fialová hover
  colors[ImGuiCol_FrameBgActive] = ImVec4 (0.60f, 0.30f, 1.00f, 0.70f);  // Jasná fialová active
  colors[ImGuiCol_TitleBg] = ImVec4 (0.25f, 0.10f, 0.50f, 0.90f);
  colors[ImGuiCol_TitleBgActive] = ImVec4 (0.45f, 0.20f, 0.80f, 0.95f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4 (0.15f, 0.05f, 0.35f, 0.80f);
  colors[ImGuiCol_MenuBarBg] = ImVec4 (0.10f, 0.05f, 0.18f, 0.90f);
  colors[ImGuiCol_ScrollbarBg] = ImVec4 (0.06f, 0.03f, 0.12f, 0.70f);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4 (0.50f, 0.20f, 0.80f, 0.60f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4 (0.60f, 0.30f, 0.90f, 0.80f);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4 (0.70f, 0.40f, 1.00f, 1.00f);
  colors[ImGuiCol_CheckMark] = ImVec4 (0.80f, 0.40f, 1.00f, 1.00f); // Světle fialová
  colors[ImGuiCol_SliderGrab] = ImVec4 (0.60f, 0.30f, 0.90f, 0.80f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4 (0.80f, 0.50f, 1.00f, 1.00f);
  colors[ImGuiCol_Button] = ImVec4 (0.20f, 0.10f, 0.35f, 0.80f);
  colors[ImGuiCol_ButtonHovered] = ImVec4 (0.45f, 0.20f, 0.75f, 0.80f); // Střední fialová hover
  colors[ImGuiCol_ButtonActive] = ImVec4 (0.65f, 0.35f, 1.00f, 0.90f);  // Jasná fialová active
  colors[ImGuiCol_Header] = ImVec4 (0.35f, 0.15f, 0.65f, 0.70f);
  colors[ImGuiCol_HeaderHovered] = ImVec4 (0.50f, 0.25f, 0.80f, 0.80f);
  colors[ImGuiCol_HeaderActive] = ImVec4 (0.70f, 0.40f, 1.00f, 0.90f);
  colors[ImGuiCol_Separator] = ImVec4 (0.45f, 0.20f, 0.75f, 0.60f);
  colors[ImGuiCol_SeparatorHovered] = ImVec4 (0.60f, 0.30f, 0.90f, 0.80f);
  colors[ImGuiCol_SeparatorActive] = ImVec4 (0.75f, 0.45f, 1.00f, 1.00f);
  colors[ImGuiCol_ResizeGrip] = ImVec4 (0.55f, 0.25f, 0.85f, 0.60f);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4 (0.65f, 0.35f, 0.95f, 0.80f);
  colors[ImGuiCol_ResizeGripActive] = ImVec4 (0.80f, 0.50f, 1.00f, 1.00f);
  colors[ImGuiCol_Tab] = ImVec4 (0.25f, 0.10f, 0.45f, 0.70f);
  colors[ImGuiCol_TabHovered] = ImVec4 (0.50f, 0.25f, 0.80f, 0.80f);
  colors[ImGuiCol_TabActive] = ImVec4 (0.65f, 0.35f, 0.95f, 0.90f);
  colors[ImGuiCol_TabUnfocused] = ImVec4 (0.15f, 0.05f, 0.30f, 0.60f);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4 (0.35f, 0.15f, 0.60f, 0.70f);
  colors[ImGuiCol_PlotLines] = ImVec4 (0.70f, 0.30f, 1.00f, 1.00f); // Světle fialová
  colors[ImGuiCol_PlotLinesHovered] = ImVec4 (0.80f, 0.50f, 1.00f, 1.00f);
  colors[ImGuiCol_PlotHistogram] = ImVec4 (0.60f, 0.20f, 0.90f, 1.00f); // Střední fialová
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4 (0.75f, 0.40f, 1.00f, 1.00f);
  colors[ImGuiCol_TextSelectedBg] = ImVec4 (0.60f, 0.30f, 0.90f, 0.35f);
  colors[ImGuiCol_DragDropTarget] = ImVec4 (0.75f, 0.40f, 1.00f, 0.80f);
  colors[ImGuiCol_NavHighlight] = ImVec4 (0.65f, 0.30f, 1.00f, 0.80f);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4 (0.90f, 0.80f, 1.00f, 0.70f);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4 (0.20f, 0.10f, 0.40f, 0.20f);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4 (0.05f, 0.02f, 0.12f, 0.60f);
}

// Main loop for the platform
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
    float time = SDL_GetTicks () / 1000.0f; // Get time in seconds
    renderBackground (time);

    ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());
    SDL_GL_SwapWindow (window_);
  }
}

// Scale ImGui based on user-defined scale factor
void PlatformManager::scaleImGui (int userScaleFactor) {
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

  // and then apply scaling
  ImGui::GetStyle ().ScaleAllSizes (scalingFactor);
}

// Render the background using the shader program
void PlatformManager::renderBackground (float deltaTime) {
  GLboolean depthTestEnabled;
  glGetBooleanv (GL_DEPTH_TEST, &depthTestEnabled);
  glDisable (GL_DEPTH_TEST);

  glUseProgram (shaderProgram_);

#if defined(IMGUI_IMPL_OPENGL_ES3) \
    || (!defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3))
  // WebGL 2.0 / OpenGL ES 3.0 or Desktop OpenGL - VAO is available
  glBindVertexArray (vao_);
#else
  // WebGL 1.0 / OpenGL ES 2.0 - VAO not available, bind buffers manually
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

// Print the overlay window with the current content
void PlatformManager::printOverlayWindow () {
  bool showOverlay = true;
  ImGui::PushID ("OverlayWindow");
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize
                                 | ImGuiWindowFlags_NoSavedSettings
                                 | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
  ImGui::Begin ("Overlay", &showOverlay, windowFlags);
  ImGui::Text ("%s", this->getOverlayContent ().c_str ());
  ImGui::End ();
  ImGui::PopID ();
}

// Initialize input handler callbacks
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

// Handle errors for SDL, OpenGL, and ImGui ...
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


