#ifndef __EMSCRIPTENPLATFORM_H__
#define __EMSCRIPTENPLATFORM_H__

#include "PlatformManager.hpp"

class EmscriptenPlatform : public PlatformManager {

  float userConfigurableScale_ = 2.0f;

public:
  EmscriptenPlatform () = default;
  ~EmscriptenPlatform () override = default;

  void initialize () override;
  void shutdown () override;
  void createSDL2Window (const char* title, int width, int height) override;
  void updateWindowSize () override;
  void createOpenGLContext () override;
  void setSwapInterval (int interval);
  void initializeGLEW () override;
  void setupShaders () override;
  GLuint compileShader (const char* shaderSource, GLenum shaderType) override;
  void renderBackground (float deltaTime) override;
  void initializeImGui () override;
  void scaleImGui () override;
};


#endif // __EMSCRIPTENPLATFORM_H__