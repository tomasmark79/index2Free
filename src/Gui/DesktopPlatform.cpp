#include "DesktopPlatform.hpp"

#include <Shaders/dyinguniverse/vertex_def.hpp>
#include <Shaders/dyinguniverse/fragment_def.hpp>

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
  updateWindowSize ();
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

void DesktopPlatform::updateWindowSize () {
  int width, height;
  SDL_GetWindowSize (window_, &width, &height);
  if (width != windowWidth_ || height != windowHeight_) {
    windowWidth_ = width;
    windowHeight_ = height;
    io_->DisplaySize = ImVec2 ((float)windowWidth_, (float)windowHeight_);
    io_->DisplayFramebufferScale = ImVec2 (1.0f, 1.0f);
  }
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
