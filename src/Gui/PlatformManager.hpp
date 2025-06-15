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

void initializePlatform ();
void shutdownPlatform ();

class PlatformManager {
public:
  PlatformManager () = default;
  virtual ~PlatformManager () = default;
  virtual void initialize () = 0;
  virtual void shutdown () = 0;

  PlatformManager (const PlatformManager&) = delete;
  PlatformManager& operator= (const PlatformManager&) = delete;
  PlatformManager (PlatformManager&&) = delete;
  PlatformManager& operator= (PlatformManager&&) = delete;

protected:
  const float BASE_FONT_SIZE = 16.0f;
  SDL_Window* window_ = nullptr;
  int windowWidth_ = 1920;
  int windowHeight_ = 1080;
  SDL_GLContext glContext_ = nullptr;
  ImGuiContext* imguiContext_ = nullptr;
  ImGuiIO* io_;
  ImGuiStyle* style_;
  const char* glsl_version_ = "#version 130"; // Default GLSL version

  virtual void createSDL2Window (const char* title, int width, int height) = 0;
  virtual void createOpenGLContext () = 0;
  void decideOpenGLVersion ();
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

#endif