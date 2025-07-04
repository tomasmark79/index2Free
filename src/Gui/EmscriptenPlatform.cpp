#include "EmscriptenPlatform.hpp"

#include <Shaders/dyinguniverse/vertex_def.hpp>
#include <Shaders/dyinguniverse/fragment_def.hpp>

#include <emscripten.h>
#include <emscripten/html5.h>

void EmscriptenPlatform::initialize () {
  createSDL2Window ("Default SDL2 Window", 1920, 1080);
  createOpenGLContext ();
  // setSwapInterval (1); // Enable vsync // not available in Emscripten
  initializeGLEW (); // // GLEW is not needed in Emscripten
  setupQuad ();
  setupShaders ();
  initializeImGui ();
  ImGuiStyle& style = ImGui::GetStyle ();
  setupImGuiStyle (style);
  scaleImGui ();
  updateWindowSize ();
  initInputHandlerCallbacks ();
  
  // In Emscripten, we need to set up the main loop differently
  emscripten_set_main_loop_arg([](void* userData) {
    EmscriptenPlatform* platform = static_cast<EmscriptenPlatform*>(userData);
    platform->frameStep();
  }, this, 0, 1);
}

void EmscriptenPlatform::shutdown () {
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

void EmscriptenPlatform::createSDL2Window (const char* title, int width, int height) {

  // IME hint not typically needed in web browsers

  if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    handleSDLError ("Failed to initialize SDL");
    return;
  }

  // Needed before creating the window
  SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);

  // For Emscripten, we don't need HIGHDPI flag as it's handled differently
  SDL_WindowFlags windowFlags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  window_ = SDL_CreateWindow (title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
                              height, windowFlags);
  if (!window_) {
    handleSDLError ("Failed to create window");
  }
  windowWidth_ = width;
  windowHeight_ = height;
}

void EmscriptenPlatform::updateWindowSize () {
  int width, height;
  SDL_GetWindowSize (window_, &width, &height);
  if (width != windowWidth_ || height != windowHeight_) {
    windowWidth_ = width;
    windowHeight_ = height;
    io_->DisplaySize = ImVec2 ((float)windowWidth_, (float)windowHeight_);
    io_->DisplayFramebufferScale = ImVec2 (1.0f, 1.0f);
  }
}

void EmscriptenPlatform::createOpenGLContext () {
  decideOpenGLVersion ();
  glContext_ = SDL_GL_CreateContext (window_);
  if (!glContext_) {
    handleSDLError ("Failed to create OpenGL context");
  }
  SDL_GL_MakeCurrent (window_, glContext_);
}

void EmscriptenPlatform::setSwapInterval (int interval) {
  // SDL_GL_SetSwapInterval may not work as expected in Emscripten/WebGL
  // Browser controls vsync, but we can still try to set it
  SDL_GL_SetSwapInterval (interval);
}

void EmscriptenPlatform::initializeGLEW () {
  // GLEW is not needed in Emscripten, OpenGL ES functions are available directly
  return;
}

void EmscriptenPlatform::setupShaders () {
  // Use OpenGL ES shaders for Emscripten
  #if defined(IMGUI_IMPL_OPENGL_ES3)
    // WebGL 2.0 / OpenGL ES 3.0
    GLuint vertexShader = compileShader (vertexShader300, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader (fragmentShader300, GL_FRAGMENT_SHADER);
  #else
    // WebGL 1.0 / OpenGL ES 2.0
    GLuint vertexShader = compileShader (vertexShader200, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader (fragmentShader200, GL_FRAGMENT_SHADER);
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
    glDeleteShader (vertexShader);
    glDeleteShader (fragmentShader);
    return;
  }

  // Clean up shaders after linking
  glDeleteShader (vertexShader);
  glDeleteShader (fragmentShader);

  // Setup VAO, VBO, EBO here if needed
}

GLuint EmscriptenPlatform::compileShader (const char* shaderSource, GLenum shaderType) {
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

void EmscriptenPlatform::renderBackground (float deltaTime) {
  // float width = (float)windowWidth_;
  // float height = (float)windowHeight_;
  // glViewport (0, 0, width, height);
  GLboolean depthTestEnabled;
  glGetBooleanv (GL_DEPTH_TEST, &depthTestEnabled);
  glDisable (GL_DEPTH_TEST);

  glUseProgram (shaderProgram_);
  
#if defined(IMGUI_IMPL_OPENGL_ES3) || (!defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3))
  // OpenGL ES 3.0+ or desktop OpenGL - VAO is available
  glBindVertexArray (vao_);
#else
  // OpenGL ES 2.0 - VAO not available, bind buffers manually
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

void EmscriptenPlatform::initializeImGui () {
  IMGUI_CHECKVERSION ();
  ImGui::CreateContext ();
  io_ = &ImGui::GetIO ();
  io_->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io_->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

  ImGui_ImplSDL2_InitForOpenGL (window_, glContext_);
  ImGui_ImplOpenGL3_Init (glsl_version_);
}

void EmscriptenPlatform::scaleImGui () {
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

void EmscriptenPlatform::frameStep() {
  SDL_Event event;
  
  // Process all pending events
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    bool shouldExit = inputHandler.processEvent(event);
    if (shouldExit) {
      // In Emscripten, we can't just exit the loop, we need to cancel the main loop
      emscripten_cancel_main_loop();
      return;
    }
  }

  // Start new ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  // Show demo window
  bool showDemo = true;
  if (showDemo)
    ImGui::ShowDemoWindow(&showDemo);

  // Render GUI
  ImGui::Render();
  glViewport(0, 0, windowWidth_, windowHeight_);
  glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Render background shader
  float time = SDL_GetTicks() / 1000.0f;
  renderBackground(time);

  // Render ImGui
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(window_);
}
