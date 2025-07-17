#ifndef __EMSCRIPTENPLATFORM_H__
#define __EMSCRIPTENPLATFORM_H__

#include "PlatformManager.hpp"

#ifdef __EMSCRIPTEN__
  #include <emscripten.h>
  #include <emscripten/html5.h>
#endif

class EmscriptenPlatform : public PlatformManager {

  float userConfigurableScale_ = 1.0f;
  float devicePixelRatio_ = 1.0f;

public:
  EmscriptenPlatform () = default;
  ~EmscriptenPlatform () override = default;

  void initialize () override;
  void shutdown () override;
  void createSDL2Window (const char* title, int width, int height) override;
  void updateWindowSize () override;
  void createOpenGLContext () override;
  void initializeGLEW () override {};
  void setSwapInterval (int interval) {};
  void SDL_GL_SetSwapInterval (int interval);
  void setupShaders () override;
  GLuint compileShader (const char* shaderSource, GLenum shaderType) override;
  void renderBackground (float deltaTime) override;
  void initializeImGui () override;
  void showScaleFactor () override;
  void scaleImGui () override;

#ifdef __EMSCRIPTEN__
  void frameStep (); // Single frame step for Emscripten main loop
#endif
};

#endif // __EMSCRIPTENPLATFORM_H__