#ifndef DESKTOPPLATFORM_HPP
#define DESKTOPPLATFORM_HPP

#include "PlatformManager.hpp"

class DesktopPlatform : public PlatformManager {

  float userConfigurableScale_ = 2.0f;

public:
  DesktopPlatform () = default;
  ~DesktopPlatform () override = default;

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

#endif