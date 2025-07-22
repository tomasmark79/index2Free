#include "PlatformManager.hpp"
#include "Logger/Logger.hpp"
#include <memory>
#include <fstream>

#include "GuiStrings.hpp"
#include "../Shaders/ShaderConvertor.hpp"

#ifdef __EMSCRIPTEN__
  #include <emscripten.h>
#endif

// Static platform instance
#if defined(__EMSCRIPTEN__)
  #include "EmscriptenPlatform.hpp"
static std::unique_ptr<EmscriptenPlatform> gPlatform = nullptr;
#else
  #include "DesktopPlatform.hpp"
static std::unique_ptr<DesktopPlatform> gPlatform = nullptr;
#endif

bool showDemo = false;
bool showOverlay = true;

#include <Shaders/Shadertoy/Happyjumping.hpp>
#include <Shaders/Shadertoy/Seascape.hpp>
#include <Shaders/Shadertoy/Synthwave.hpp>
#include <Shaders/Shadertoy/Glasscube.hpp>
#include <Shaders/Shadertoy/Singularity.hpp>
#include <Shaders/Shadertoy/Fractaltrees.hpp>
#include <Shaders/Shadertoy/Fireflame.hpp>
#include <Shaders/Shadertoy/Tunnel.hpp>
#include <Shaders/Shadertoy/Sunset.hpp>
#include <Shaders/Shadertoy/Sunset2.hpp>
#include <Shaders/Shadertoy/Anothercube.hpp>
#include <Shaders/Shadertoy/Abug.hpp>
#include <Shaders/Shadertoy/Bluemoonocean.hpp>

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
  // Simple shader switcher - change this number to switch shaders (0-9)
  int currentShader = 12; // Change this to switch between shaders
  
  std::string shaderToUse; 
  switch(currentShader) {
    case 0: shaderToUse = fragmentShaderToyHappyjumping; break;
    case 1: shaderToUse = fragmentShaderToySeascape; break;
    case 2: shaderToUse = fragmentShaderToySynthwave; break;
    case 3: shaderToUse = fragmentShaderToyGlasscube; break;
    case 4: shaderToUse = fragmentShaderToySingularity; break;
    case 5: shaderToUse = fragmentShaderToyFractaltrees; break;
    case 6: shaderToUse = fragmentShaderToyFireflame; break;
    case 7: shaderToUse = fragmentShaderToyTunnel; break;
    case 8: shaderToUse = fragmentShaderToySunset; break;
    case 9: shaderToUse = fragmentShaderToySunset2; break;
    case 10: shaderToUse = fragmentShaderToyAnothercube; break;
    case 11: shaderToUse = fragmentShaderToyAbug; break;
    case 12: shaderToUse = fragmentShaderToyBluemoonocean; break;

    default: shaderToUse = fragmentShaderToyHappyjumping; break;

  }

  // Determine target platform based on OpenGL version
  ShaderTarget target = ShaderTarget::Desktop330;
#if defined(IMGUI_IMPL_OPENGL_ES2)
  target = ShaderTarget::WebGL1;
#elif defined(IMGUI_IMPL_OPENGL_ES3)
  target = ShaderTarget::WebGL2;
#elif defined(__APPLE__)
  target = ShaderTarget::Desktop330;
#else
  target = ShaderTarget::Desktop330;
#endif

  // Use ShaderConvertor to convert the ShaderToy code
  ShaderConvertor convertor;
  ShaderConversionResult result = convertor.convertFromShaderToy(shaderToUse, target);
  
  if (!result.success) {
    LOG_E_STREAM << "ShaderConvertor failed: " << result.errorMessage << std::endl;
    // Fall back to basic error shader
    shaderProgram_ = 0;
    return;
  }

  LOG_I_STREAM << "Using ShaderConvertor - target: " << static_cast<int>(target) << std::endl;
  LOG_I_STREAM << "Using ShaderConvertor - target: " << static_cast<int>(target) << std::endl;
  LOG_I_STREAM << "Vertex shader length: " << result.vertexShader.length() << std::endl;
  LOG_I_STREAM << "Fragment shader length: " << result.fragmentShader.length() << std::endl;
  
  // Debug: Save converted shaders to files for inspection
  std::ofstream debugVertFile("converted_vertex_shader.glsl");
  if (debugVertFile.is_open()) {
    debugVertFile << result.vertexShader;
    debugVertFile.close();
  }
  
  std::ofstream debugFragFile("converted_fragment_shader.glsl");
  if (debugFragFile.is_open()) {
    debugFragFile << result.fragmentShader;
    debugFragFile.close();
    LOG_I_STREAM << "Debug: Converted shaders saved to converted_*_shader.glsl" << std::endl;
  }

  GLuint vertexShader = compileShader(result.vertexShader.c_str(), GL_VERTEX_SHADER);
  GLuint fragmentShader = compileShader(result.fragmentShader.c_str(), GL_FRAGMENT_SHADER);

  LOG_I_STREAM << "Vertex shader compilation result: " << (vertexShader != 0 ? "SUCCESS" : "FAILED") << std::endl;
  LOG_I_STREAM << "Fragment shader compilation result: " << (fragmentShader != 0 ? "SUCCESS" : "FAILED") << std::endl;

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
    GLint logLength;
    glGetProgramiv (shaderProgram_, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<char> infoLog (logLength);
    glGetProgramInfoLog (shaderProgram_, logLength, nullptr, infoLog.data ());

    handleError ("Failed to link shader program", infoLog.data ());
    // IMPORTANT: Don't return here - set shaderProgram_ to 0!
    shaderProgram_ = 0;
    return;
  }

  // Clean up shaders after linking
  glDeleteShader (vertexShader);
  glDeleteShader (fragmentShader);
}

// Compile shader from source code - Returns the shader ID or 0 on failure
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
    
    // Debug: Log shader compilation error details
    std::string shaderTypeStr = (shaderType == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
    LOG_E_STREAM << "=== " << shaderTypeStr << " SHADER COMPILATION ERROR ===" << std::endl;
    LOG_E_STREAM << "Error: " << infoLog.data() << std::endl;
    LOG_E_STREAM << "=== SHADER SOURCE ===" << std::endl;
    LOG_E_STREAM << shaderSource << std::endl;
    LOG_E_STREAM << "=== END SHADER SOURCE ===" << std::endl;
    
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

  defaultStyle_ = ImGui::GetStyle ();
  style_ = ImGui::GetStyle ();
  applyStyleLila (style_, DEFAULT_WINDOW_OPACITY); // Apply the Lila style with full opacity
}

// Apply the Lila style to ImGui
void PlatformManager::applyStyleLila (ImGuiStyle& style, float alpha) {
  ImVec4* colors = style.Colors;

  style.WindowRounding = 8.0f;
  style.FrameRounding = 6.0f;
  style.GrabRounding = 6.0f;
  style.ScrollbarRounding = 6.0f;
  style.FrameBorderSize = 1.5f;
  style.WindowBorderSize = 2.0f;
  style.WindowPadding = ImVec2 (12.0f, 12.0f);
  style.FramePadding = ImVec2 (8.0f, 4.0f);

  // Tmavý futuristický theme - FIALOVÁ PALETA
  colors[ImGuiCol_Text] = ImVec4 (0.95f, 0.90f, 1.00f, 1.00f * alpha);
  colors[ImGuiCol_TextDisabled] = ImVec4 (0.50f, 0.40f, 0.60f, 0.80f * alpha);
  colors[ImGuiCol_WindowBg] = ImVec4 (0.08f, 0.05f, 0.15f, 0.70f * alpha);
  colors[ImGuiCol_ChildBg] = ImVec4 (0.06f, 0.03f, 0.12f, 0.60f * alpha);
  colors[ImGuiCol_PopupBg] = ImVec4 (0.12f, 0.08f, 0.20f, 0.80f * alpha);
  colors[ImGuiCol_Border] = ImVec4 (0.60f, 0.20f, 1.00f, 0.80f * alpha);
  colors[ImGuiCol_BorderShadow] = ImVec4 (0.30f, 0.10f, 0.60f, 0.50f * alpha);
  colors[ImGuiCol_FrameBg] = ImVec4 (0.12f, 0.08f, 0.20f, 0.80f * alpha);
  colors[ImGuiCol_FrameBgHovered] = ImVec4 (0.40f, 0.15f, 0.70f, 0.50f * alpha);
  colors[ImGuiCol_FrameBgActive] = ImVec4 (0.60f, 0.30f, 1.00f, 0.70f * alpha);
  colors[ImGuiCol_TitleBg] = ImVec4 (0.25f, 0.10f, 0.50f, 0.70f * alpha);
  colors[ImGuiCol_TitleBgActive] = ImVec4 (0.45f, 0.20f, 0.80f, 0.80f * alpha);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4 (0.15f, 0.05f, 0.35f, 0.60f * alpha);
  colors[ImGuiCol_MenuBarBg] = ImVec4 (0.10f, 0.05f, 0.18f, 0.70f * alpha);
  colors[ImGuiCol_ScrollbarBg] = ImVec4 (0.06f, 0.03f, 0.12f, 0.50f * alpha);
  colors[ImGuiCol_ScrollbarGrab] = ImVec4 (0.50f, 0.20f, 0.80f, 0.60f * alpha);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4 (0.60f, 0.30f, 0.90f, 0.80f * alpha);
  colors[ImGuiCol_ScrollbarGrabActive] = ImVec4 (0.70f, 0.40f, 1.00f, 1.00f * alpha);
  colors[ImGuiCol_CheckMark] = ImVec4 (0.80f, 0.40f, 1.00f, 1.00f * alpha);
  colors[ImGuiCol_SliderGrab] = ImVec4 (0.60f, 0.30f, 0.90f, 0.80f * alpha);
  colors[ImGuiCol_SliderGrabActive] = ImVec4 (0.80f, 0.50f, 1.00f, 1.00f * alpha);
  colors[ImGuiCol_Button] = ImVec4 (0.20f, 0.10f, 0.35f, 0.80f * alpha);
  colors[ImGuiCol_ButtonHovered] = ImVec4 (0.45f, 0.20f, 0.75f, 0.80f * alpha);
  colors[ImGuiCol_ButtonActive] = ImVec4 (0.65f, 0.35f, 1.00f, 0.90f * alpha);
  colors[ImGuiCol_Header] = ImVec4 (0.35f, 0.15f, 0.65f, 0.70f * alpha);
  colors[ImGuiCol_HeaderHovered] = ImVec4 (0.50f, 0.25f, 0.80f, 0.80f * alpha);
  colors[ImGuiCol_HeaderActive] = ImVec4 (0.70f, 0.40f, 1.00f, 0.90f * alpha);
  colors[ImGuiCol_Separator] = ImVec4 (0.45f, 0.20f, 0.75f, 0.60f * alpha);
  colors[ImGuiCol_SeparatorHovered] = ImVec4 (0.60f, 0.30f, 0.90f, 0.80f * alpha);
  colors[ImGuiCol_SeparatorActive] = ImVec4 (0.75f, 0.45f, 1.00f, 1.00f * alpha);
  colors[ImGuiCol_ResizeGrip] = ImVec4 (0.55f, 0.25f, 0.85f, 0.60f * alpha);
  colors[ImGuiCol_ResizeGripHovered] = ImVec4 (0.65f, 0.35f, 0.95f, 0.80f * alpha);
  colors[ImGuiCol_ResizeGripActive] = ImVec4 (0.80f, 0.50f, 1.00f, 1.00f * alpha);
  colors[ImGuiCol_Tab] = ImVec4 (0.25f, 0.10f, 0.45f, 0.70f * alpha);
  colors[ImGuiCol_TabHovered] = ImVec4 (0.50f, 0.25f, 0.80f, 0.80f * alpha);
  colors[ImGuiCol_TabActive] = ImVec4 (0.65f, 0.35f, 0.95f, 0.90f * alpha);
  colors[ImGuiCol_TabUnfocused] = ImVec4 (0.15f, 0.05f, 0.30f, 0.60f * alpha);
  colors[ImGuiCol_TabUnfocusedActive] = ImVec4 (0.35f, 0.15f, 0.60f, 0.70f * alpha);
  colors[ImGuiCol_PlotLines] = ImVec4 (0.70f, 0.30f, 1.00f, 1.00f * alpha);
  colors[ImGuiCol_PlotLinesHovered] = ImVec4 (0.80f, 0.50f, 1.00f, 1.00f * alpha);
  colors[ImGuiCol_PlotHistogram] = ImVec4 (0.60f, 0.20f, 0.90f, 1.00f * alpha);
  colors[ImGuiCol_PlotHistogramHovered] = ImVec4 (0.75f, 0.40f, 1.00f, 1.00f * alpha);
  colors[ImGuiCol_TextSelectedBg] = ImVec4 (0.60f, 0.30f, 0.90f, 0.35f * alpha);
  colors[ImGuiCol_DragDropTarget] = ImVec4 (0.75f, 0.40f, 1.00f, 0.80f * alpha);
  colors[ImGuiCol_NavHighlight] = ImVec4 (0.65f, 0.30f, 1.00f, 0.80f * alpha);
  colors[ImGuiCol_NavWindowingHighlight] = ImVec4 (0.90f, 0.80f, 1.00f, 0.70f * alpha);
  colors[ImGuiCol_NavWindowingDimBg] = ImVec4 (0.20f, 0.10f, 0.40f, 0.20f * alpha);
  colors[ImGuiCol_ModalWindowDimBg] = ImVec4 (0.05f, 0.02f, 0.12f, 0.40f * alpha);
}

// Main loop for the platform
void PlatformManager::mainLoop () {
  bool done = false;

  // #ifdef __EMSCRIPTEN__
  //   emscripten_set_main_loop_timing (EM_TIMING_RAF, 0);
  // #endif

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

    if (showDemo)
      ImGui::ShowDemoWindow (&showDemo);

    if (showOverlay) {
      printOverlayWindow ();
    }

    // Render GUI
    ImGui::Render ();
    glViewport (0, 0, windowWidth_, windowHeight_);
    glClearColor (0.45f, 0.55f, 0.60f, 1.00f);
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render background shader - cumulative time
    static float totalTime = 0.0f;
    static unsigned int lastTime = SDL_GetTicks ();
    unsigned int currentTime = SDL_GetTicks ();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    totalTime += deltaTime;
    lastTime = currentTime;
    renderBackground (totalTime); // Pass cumulative time, not delta

    ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());
    SDL_GL_SwapWindow (window_);
  }
}

// Scale ImGui based on user-defined scale factor
void PlatformManager::scaleImGui (float userScaleFactor) {
  float scalingFactor = userScaleFactor;
  float fontSize = BASE_FONT_SIZE * scalingFactor * devicePixelRatio_;
  fontSize = std::max (1.0f, roundf (fontSize));
  ImFontConfig fontCfg = {};
  fontCfg.RasterizerDensity = scalingFactor;
  static const ImWchar czRanges[]
      = { 0x0020, 0x00FF, 0x0100, 0x017F, 0x0200, 0x024F, 0x0401, 0x045F, 0x0402, 0x045F,
          0x0403, 0x045F, 0x0404, 0x045F, 0x0405, 0x045F, 0x0406, 0x045F, 0x0407, 0x045F,
          0x0408, 0x045F, 0x0409, 0x045F, 0x040A, 0x045F, 0x040B, 0x045F, 0x040C, 0x045F,
          0x040D, 0x045F, 0x040E, 0x045F, 0x040F, 0x045F, 0 };
  std::filesystem::path fnt = AssetContext::getAssetsPath () / "fonts" / "Comfortaa-Light.otf";

  // ImGui::GetIO ().FontGlobalScale = scalingFactor;
  io_->Fonts->Clear ();
  io_->Fonts->AddFontFromFileTTF (fnt.string ().c_str (), fontSize, &fontCfg, czRanges);
  io_->Fonts->Build ();

  // Always start scale from default style sizes
  ImGui::GetStyle () = defaultStyle_;
  ImGui::GetStyle ().ScaleAllSizes (scalingFactor);

  // At least apply the style to the current ImGui context
  applyStyleLila (ImGui::GetStyle (), DEFAULT_WINDOW_OPACITY); // Full opacity for the style
}

// Render the background using the shader program
void PlatformManager::renderBackground (float totalTime) {
  if (shaderProgram_ == 0) {
    return; // No shader program available
  }

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

  // uniform vec3      iResolution;           // viewport resolution (in pixels)
  // uniform float     iTime;                 // shader playback time (in seconds)
  // uniform float     iTimeDelta;            // render time (in seconds)
  // uniform float     iFrameRate;            // shader frame rate
  // uniform int       iFrame;                // shader playback frame
  // uniform float     iChannelTime[4];       // channel playback time (in seconds)
  // uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)
  // uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
  // uniform samplerXX iChannel0..3;          // input channel. XX = 2D/Cube
  // uniform vec4      iDate;                 // (year, month, day, time in seconds)

  GLint iResolutionLoc = glGetUniformLocation (shaderProgram_, "iResolution");
  GLint iTimeLoc = glGetUniformLocation (shaderProgram_, "iTime");
  GLint iTimeDeltaLoc = glGetUniformLocation (shaderProgram_, "iTimeDelta");
  GLint iFrameRateLoc = glGetUniformLocation (shaderProgram_, "iFrameRate");
  GLint iFrameLoc = glGetUniformLocation (shaderProgram_, "iFrame");
  GLint iChannelTimeLoc = glGetUniformLocation (shaderProgram_, "iChannelTime");
  GLint iChannelResolutionLoc = glGetUniformLocation (shaderProgram_, "iChannelResolution");
  GLint iMouseLoc = glGetUniformLocation (shaderProgram_, "iMouse");
  GLint iDateLoc = glGetUniformLocation (shaderProgram_, "iDate");
  GLint iChannel0Loc = glGetUniformLocation (shaderProgram_, "iChannel0");
  GLint iChannel1Loc = glGetUniformLocation (shaderProgram_, "iChannel1");
  GLint iChannel2Loc = glGetUniformLocation (shaderProgram_, "iChannel2");
  GLint iChannel3Loc = glGetUniformLocation (shaderProgram_, "iChannel3");

  // ⚡ PERFORMANCE: Statické proměnné pro frame counter
  static int frameCount = 0;
  static float lastDeltaTime = 0.016f; // Default 60 FPS
  frameCount++;

  if (iResolutionLoc != -1)
    glUniform3f (iResolutionLoc, (float)windowWidth_, (float)windowHeight_, 1.0f);
  if (iTimeLoc != -1)
    glUniform1f (iTimeLoc, totalTime);
  if (iTimeDeltaLoc != -1)
    glUniform1f (iTimeDeltaLoc, lastDeltaTime);
  if (iFrameRateLoc != -1)
    glUniform1f (iFrameRateLoc, ImGui::GetIO ().Framerate);
  if (iFrameLoc != -1)
    glUniform1i (iFrameLoc, frameCount);
  // iChannelTime[4]
  if (iChannelTimeLoc != -1) {
    float channelTime[4] = { totalTime, totalTime, totalTime, totalTime };
    glUniform1fv (iChannelTimeLoc, 4, channelTime);
  }
  // iChannelResolution[4]
  if (iChannelResolutionLoc != -1) {
    float channelRes[12] = { (float)windowWidth_, (float)windowHeight_, 1.0f,
                             (float)windowWidth_, (float)windowHeight_, 1.0f,
                             (float)windowWidth_, (float)windowHeight_, 1.0f,
                             (float)windowWidth_, (float)windowHeight_, 1.0f };
    glUniform3fv (iChannelResolutionLoc, 4, channelRes);
  }
  // iMouse
  if (iMouseLoc != -1)
    glUniform4f (iMouseLoc, 0.0f, 0.0f, 0.0f, 0.0f); // TODO: real mouse coords
  // iDate
  if (iDateLoc != -1) {
    time_t now = time (nullptr);
    struct tm* t = localtime (&now);
    float seconds = (float)t->tm_hour * 3600.0f + (float)t->tm_min * 60.0f + (float)t->tm_sec;
    glUniform4f (iDateLoc, (float)(1900 + t->tm_year), (float)(1 + t->tm_mon), (float)t->tm_mday,
                 seconds);
  }
  // iChannel0..3 (sampler2D/samplerCube) - demo: bind 0
  if (iChannel0Loc != -1)
    glUniform1i (iChannel0Loc, 0);
  if (iChannel1Loc != -1)
    glUniform1i (iChannel1Loc, 0);
  if (iChannel2Loc != -1)
    glUniform1i (iChannel2Loc, 0);
  if (iChannel3Loc != -1)
    glUniform1i (iChannel3Loc, 0);
  if (iTimeLoc != -1) {
    glUniform1f (iTimeLoc, totalTime); // Dynamický čas pro animace
    // glUniform1f (iTimeLoc, 5.0f); // ⚡ STATIC TIME: Completely frozen time for testing
  }
  if (iTimeDeltaLoc != -1)
    glUniform1f (iTimeDeltaLoc, lastDeltaTime);
  if (iFrameLoc != -1)
    glUniform1i (iFrameLoc, frameCount);
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

// Get the content for the overlay window
std::string PlatformManager::getOverlayContent () {
  std::string oC = "";
  oC += fmt::format ("{:.3f} ms/frame {:.1f} FPS\n", 1000.0f / ImGui::GetIO ().Framerate,
                     ImGui::GetIO ().Framerate);

  oC += fmt::format ("Drawable Size: {} x {}\n", windowWidth_, windowHeight_);
  oC += fmt::format ("ImGui Display Size: {:.0f} x {:.0f}\n", io_->DisplaySize.x,
                     io_->DisplaySize.y);
  oC += fmt::format ("ImGui Display Framebuffer Scale: {:.2f} x {:.2f}\n",
                     io_->DisplayFramebufferScale.x, io_->DisplayFramebufferScale.y);
  oC += fmt::format ("User Scale Factor: {:.2f}\n", userScaleFactor);
  oC += fmt::format ("Device Pixel Ratio: {:.2f}\n", devicePixelRatio_);
  oC += fmt::format ("Base Font Size: {:.2f}\n", BASE_FONT_SIZE);
  oC += fmt::format ("Font Size: {:.2f}\n", io_->FontGlobalScale * BASE_FONT_SIZE);

  return oC;
}

// Print the overlay window with the current content
void PlatformManager::printOverlayWindow () {
  static float lastUpdateTime = 0.0f;
  static std::string cachedOverlayContent = "";
  float currentTime = SDL_GetTicks () / 1000.0f;

  // Update content only once per second
  if (currentTime - lastUpdateTime >= 1.0f) {
    cachedOverlayContent = this->getOverlayContent ();
    lastUpdateTime = currentTime;
  }

  bool showOverlay = true;
  ImGui::PushID ("OverlayWindow");
  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_AlwaysAutoResize
                                 | ImGuiWindowFlags_NoSavedSettings
                                 | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
  ImGui::Begin ("Overlay", &showOverlay, windowFlags);
  ImGui::Text ("%s", cachedOverlayContent.c_str ());
  ImGui::End ();
  ImGui::PopID ();
}

// Initialize input handler callbacks
void PlatformManager::initInputHandlerCallbacks () {
  inputHandler.setScaleCallback ([&] (float scaleFactor) {
    this->userScaleFactor += scaleFactor;
    this->userScaleFactor = std::max (DEFAULT_MIN_ZOOM, this->userScaleFactor);
    this->userScaleFactor = std::min (DEFAULT_MAX_ZOOM, this->userScaleFactor);
    this->scaleImGui (this->userScaleFactor);
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
