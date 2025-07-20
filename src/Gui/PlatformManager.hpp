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

#define DEFAULT_WINDOW_OPACITY (float)0.95f
#define DEFAULT_MIN_ZOOM (float)0.35f
#define DEFAULT_MAX_ZOOM (float)5.0f
#define DEFAULT_FPS_ 60
#define DEFAULT_WINDOW_WIDTH 1920
#define DEFAULT_WINDOW_HEIGHT 1080
#define DEFAULT_SCALING_FACTOR_DESKTOP (float)2.0f
#define DEFAULT_SCALING_FACTOR_EMSCRIPTEN (float)1.0f
#define FALLBACK_DEVICE_PIXEL_RATIO (float)1.0f
#define BASE_FONT_SIZE (float)16.0f

void initializePlatform ();

class PlatformManager {

public:
  PlatformManager () = default;
  virtual ~PlatformManager () = default;

protected:
  int windowWidth_ = DEFAULT_WINDOW_WIDTH;
  int windowHeight_ = DEFAULT_WINDOW_HEIGHT;

#ifdef __EMSCRIPTEN__
  float userScaleFactor = DEFAULT_SCALING_FACTOR_EMSCRIPTEN;
#else
  float userScaleFactor = DEFAULT_SCALING_FACTOR_DESKTOP;
#endif
  float devicePixelRatio_ = FALLBACK_DEVICE_PIXEL_RATIO;

  InputHandler inputHandler;
  SDL_GLContext glContext_ = nullptr;
  SDL_Window* window_ = nullptr;
  GLuint vao_, vbo_, ebo_, shaderProgram_;
  ImGuiContext* imguiContext_ = nullptr;
  ImGuiIO* io_;

  ImGuiStyle defaultStyle_, style_;

  const char* glsl_version_ = "#version 130"; // Default GLSL version

public:
  virtual void initialize () = 0;
  void shutdown ();

protected:
  virtual void updateWindowSize () = 0;

protected:
  void createSDL2Window (const char* title, int width, int height);
  void createOpenGLContext (int swapInterval);
  void setupShaders ();
  GLuint compileShader (const char* shaderSource, GLenum shaderType);
  void decideOpenGLVersion ();
  void setupQuad ();
  void initializeImGui ();
  void applyStyleLila (ImGuiStyle& style, float alpha = 1.0f);
  void mainLoop ();
  void scaleImGui (float userScaleFactor = 1.0f);
  void renderBackground (float deltaTime);
  std::string getOverlayContent ();
  void printOverlayWindow ();
  void initInputHandlerCallbacks (); // TODO
  void handleSDLError (const char* message) const;
  void handleGLError (const char* message) const;
  void handleImGuiError (const char* message) const;
  void handleError (const char* message) const;
  void handleError (const char* message, const char* error) const;
  void handleError (const char* message, int errorCode) const;
};

#endif // __PLATFORMMANAGER_H__