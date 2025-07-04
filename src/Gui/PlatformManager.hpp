#ifndef __PLATFORMMANAGER_H__
#define __PLATFORMMANAGER_H__

#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <Utils/Utils.hpp>
#include "TextureTools.hpp"
#include "InputHandler.hpp"

#include <SDL.h>
#include <SDL_image.h>

#include "bindings/imgui_impl_opengl3.h"
#include "bindings/imgui_impl_sdl2.h"
#include "imgui.h"

#if defined(IMGUI_IMPL_OPENGL_ES2) || defined(IMGUI_IMPL_OPENGL_ES3)
  #include <SDL_opengles2.h>
#else
  #include <GL/glew.h>
#endif

// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>

void initializePlatform ();
void shutdownPlatform ();

class PlatformManager {

public:
  PlatformManager () = default;
  virtual ~PlatformManager () = default;

protected:
  int windowWidth_ = 1920;
  int windowHeight_ = 1080;
  const float BASE_FONT_SIZE = 16.0f;
  InputHandler inputHandler;
  SDL_GLContext glContext_ = nullptr;
  SDL_Window* window_ = nullptr;
  GLuint vao_, vbo_, ebo_, shaderProgram_;
  ImGuiContext* imguiContext_ = nullptr;
  ImGuiIO* io_;
  ImGuiStyle* style_;
  const char* glsl_version_ = "#version 130"; // Default GLSL version

public:
  virtual void initialize () = 0;
  virtual void shutdown () = 0;

protected:
  virtual void createSDL2Window (const char* title, int width, int height) = 0;
  virtual void updateWindowSize () = 0;

protected:
  void decideOpenGLVersion ();
  virtual void createOpenGLContext () = 0;
  virtual void initializeGLEW () = 0;
  void setupQuad ();
  virtual void setupShaders () = 0;
  virtual GLuint compileShader (const char* shaderSource, GLenum shaderType) = 0;
  virtual void renderBackground (float deltaTime) = 0;

protected:
  void setupImGuiStyle (ImGuiStyle& style);
  virtual void initializeImGui () = 0;
  virtual void scaleImGui () = 0;

  protected:
  void initInputHandlerCallbacks ();

protected:
  void mainLoop ();

protected:
  void handleSDLError (const char* message) const;
  void handleGLError (const char* message) const;
  void handleImGuiError (const char* message) const;
  void handleError (const char* message) const;
  void handleError (const char* message, const char* error) const;
  void handleError (const char* message, int errorCode) const;
};


#endif // __PLATFORMMANAGER_H__