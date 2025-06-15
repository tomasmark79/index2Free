#ifndef PLATFORMMANAGER_HPP
#define PLATFORMMANAGER_HPP

#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <Utils/Utils.hpp>

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

#include <stb_image.h>

// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>

void initializePlatform ();
void shutdownPlatform ();

class PlatformManager {
public:
  PlatformManager () = default;
  virtual ~PlatformManager () = default;
  virtual void initialize () = 0;
  virtual void shutdown () = 0;

protected:
  const float BASE_FONT_SIZE = 16.0f;
  SDL_Window* window_ = nullptr;
  int windowWidth_ = 1920;
  int windowHeight_ = 1080;

  SDL_GLContext glContext_ = nullptr;
  GLuint vao_, vbo_, ebo_, shaderProgram_;

  ImGuiContext* imguiContext_ = nullptr;
  ImGuiIO* io_;
  ImGuiStyle* style_;
  const char* glsl_version_ = "#version 130"; // Default GLSL version

  virtual void createSDL2Window (const char* title, int width, int height) = 0;

  void decideOpenGLVersion ();
  virtual void createOpenGLContext () = 0;
  virtual void initializeGLEW () = 0;
  void setupQuad ();
  virtual void setupShaders () = 0;
  virtual GLuint compileShader (const char* shaderSource, GLenum shaderType) = 0;
  virtual void renderBackground (float deltaTime) = 0;

  void setupImGuiStyle (ImGuiStyle& style);
  virtual void initializeImGui () = 0;
  virtual void scaleImGui () = 0;

  void mainLoop ();

  // void shutdownImGui ();
  // void destroyWindow ();
  // void destroyOpenGLContext ();

  void handleSDLError (const char* message) const;
  void handleGLError (const char* message) const;
  void handleImGuiError (const char* message) const;
  void handleError (const char* message) const;
  void handleError (const char* message, const char* error) const;
  void handleError (const char* message, int errorCode) const;
};

// X86
class DesktopPlatform : public PlatformManager {
public:
  DesktopPlatform () = default;
  ~DesktopPlatform () override = default;

  void initialize () override;
  void shutdown () override;
  void createSDL2Window (const char* title, int width, int height) override;
  void createOpenGLContext () override;
  void setSwapInterval (int interval);
  void initializeGLEW () override;
  void setupShaders () override;
  GLuint compileShader (const char* shaderSource, GLenum shaderType) override;
  void renderBackground (float deltaTime) override;

  void initializeImGui () override;
  void scaleImGui () override;

private:
  float userConfigurableScale_ = 2.0f;
};

// WebAssembly TODO: Implement EmscriptenPlatform
class EmscriptenPlatform : public PlatformManager {
public:
  EmscriptenPlatform () = default;
  ~EmscriptenPlatform () override = default;

  // void initialize () override;
  // void shutdown () override;
  // void createSDL2Window (const char* title, int width, int height) override;
  // void createOpenGLContext () override;
  // void initializeImGui () override;
};

namespace TextureLoader {
  bool LoadTextureFromMemory (const void* data, size_t data_size, SDL_Renderer* renderer,
                              SDL_Texture** out_texture, int* out_width, int* out_height);
  bool LoadTextureFromFile (const std::filesystem::path& file_path, SDL_Renderer* renderer,
                            SDL_Texture** out_texture, int* out_width, int* out_height);
} // namespace TextureLoader
#endif