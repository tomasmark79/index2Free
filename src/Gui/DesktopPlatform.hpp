#ifndef __DESKTOPPLATFORM_H__
#define __DESKTOPPLATFORM_H__

#include "PlatformManager.hpp"

class DesktopPlatform : public PlatformManager {

public:
  DesktopPlatform () = default;
  ~DesktopPlatform () override = default;

  float userConfigurableScale_ = 2.0f;

  virtual void initialize () override;
  virtual void shutdown () override;

protected:
  virtual void createSDL2Window (const char* title, int width, int height) override;
  virtual void initializeGLEW () override;
  virtual void setupShaders () override;
  virtual void renderBackground (float deltaTime) override;
  virtual void initializeImGui () override;
  virtual std::string getOverlayContent () override;
  virtual void updateWindowSize () override;
};
#endif // __DESKTOPPLATFORM_H__