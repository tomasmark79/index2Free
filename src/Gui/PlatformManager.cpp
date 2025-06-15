#include "PlatformManager.hpp"

#include <Shaders/dyinguniverse/vertex_def.hpp>
#include <Shaders/dyinguniverse/fragment_def.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void initializePlatform () {
#if defined(__EMSCRIPTEN__)
  static EmscriptenPlatform pltf;
  pltf.initialize ();
#else
  static DesktopPlatform pltf;
  pltf.initialize ();
#endif
}
void shutdownPlatform () {
#if defined(__EMSCRIPTEN__)
  static EmscriptenPlatform pltf;
  pltf.shutdown ();
#else
  static DesktopPlatform pltf;
  pltf.shutdown ();
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

  glGenVertexArrays (1, &vao_);
  glGenBuffers (1, &vbo_);
  glGenBuffers (1, &ebo_);

  glBindVertexArray (vao_);

  glBindBuffer (GL_ARRAY_BUFFER, vbo_);
  glBufferData (GL_ARRAY_BUFFER, sizeof (vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (indices), indices, GL_STATIC_DRAW);

  // Vertex attribute (location = 0, 2 floats per vertex)
  glEnableVertexAttribArray (0);
  glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof (float), (void*)0);

  glBindVertexArray (0);
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

void DesktopPlatform::setSwapInterval (int interval) {
  SDL_GL_SetSwapInterval (interval); // vsync only on desktop
}

void DesktopPlatform::initializeGLEW () {

#ifndef IMGUI_IMPL_OPENGL_ES2
  if (glewInit () != GLEW_OK) {
    printf ("Error initializing GLEW\n");
    return;
  }
#endif
}

GLuint DesktopPlatform::compileShader (const char* shaderSource, GLenum shaderType) {
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

  // Setup VAO, VBO, EBO here if needed
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

void DesktopPlatform::scaleImGui () {
  ImGuiStyle defaultStyle_;
  io_->DisplaySize = ImVec2 ((float)windowWidth_, (float)windowHeight_);
  io_->DisplayFramebufferScale = ImVec2 (1.0f, 1.0f);
  float scalingFactor = userConfigurableScale_;
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

  // Recreate font texture
  ImGui_ImplOpenGL3_DestroyFontsTexture ();
  ImGui_ImplOpenGL3_CreateFontsTexture ();

  // Reset style to defaults
  ImGui::GetStyle () = defaultStyle_;

  // and then apply scaling
  ImGui::GetStyle ().ScaleAllSizes (scalingFactor);
}

namespace TextureLoader {
  bool LoadTextureFromMemory (const void* data, size_t data_size, SDL_Renderer* renderer,
                              SDL_Texture** out_texture, int* out_width, int* out_height) {
    int image_width = 0, image_height = 0, channels = 0;
    unsigned char* image_data = stbi_load_from_memory (static_cast<const unsigned char*> (data),
                                                       static_cast<int> (data_size), &image_width,
                                                       &image_height, &channels, 4);
    if (!image_data) {
      LOG_E_FMT ("Failed to load image from memory: %s", stbi_failure_reason ());
      return false;
    }

    SDL_Surface* surface
        = SDL_CreateRGBSurfaceFrom (image_data, image_width, image_height, 32, 4 * image_width,
                                    0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if (!surface) {
      LOG_E_FMT ("Failed to create SDL surface: %s", SDL_GetError ());
      stbi_image_free (image_data);
      return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface (renderer, surface);
    if (!texture) {
      LOG_E_FMT ("Failed to create SDL texture: %s", SDL_GetError ());
      SDL_FreeSurface (surface);
      stbi_image_free (image_data);
      return false;
    }

    *out_texture = texture;
    *out_width = image_width;
    *out_height = image_height;

    SDL_FreeSurface (surface);
    stbi_image_free (image_data);
    return true;
  }

  bool LoadTextureFromFile (const std::filesystem::path& file_path, SDL_Renderer* renderer,
                            SDL_Texture** out_texture, int* out_width, int* out_height) {
    std::ifstream file (file_path, std::ios::binary | std::ios::ate);
    if (!file) {
      LOG_E_FMT ("Failed to open file: %s", file_path.string ().c_str ());
      return false;
    }
    std::streamsize file_size = file.tellg ();
    if (file_size <= 0) {
      LOG_E_FMT ("File is empty or error reading file: %s", file_path.string ().c_str ());
      return false;
    }
    file.seekg (0, std::ios::beg);

    std::vector<unsigned char> buffer (static_cast<size_t> (file_size));
    if (!file.read (reinterpret_cast<char*> (buffer.data ()), file_size)) {
      LOG_E_FMT ("Failed to read file: %s", file_path.string ().c_str ());
      return false;
    }

    return LoadTextureFromMemory (buffer.data (), buffer.size (), renderer, out_texture, out_width,
                                  out_height);
  }
} // namespace TextureLoader

void DesktopPlatform::initialize () {
  createSDL2Window ("Default SDL2 Window", 1920, 1080);
  createOpenGLContext ();
  setSwapInterval (1); // Enable vsync
  initializeGLEW ();
  setupQuad ();
  setupShaders ();
  initializeImGui ();
  ImGuiStyle& style = ImGui::GetStyle ();
  setupImGuiStyle (style);
  scaleImGui ();
  mainLoop ();
}

void PlatformManager::mainLoop () {
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

    // Render GUI
    ImGui::Render ();
    glViewport (0, 0, windowWidth_, windowHeight_);
    glClearColor (0.45f, 0.55f, 0.60f, 1.00f);
    glClear (GL_COLOR_BUFFER_BIT);

    float time = SDL_GetTicks () / 1000.0f;
    renderBackground (time);

    ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());
    SDL_GL_SwapWindow (window_);
  }
}
