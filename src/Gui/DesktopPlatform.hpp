#ifndef __DESKTOPPLATFORM_H__
#define __DESKTOPPLATFORM_H__

#include "PlatformManager.hpp"

class DesktopPlatform : public PlatformManager {

public:
  DesktopPlatform () = default;
  ~DesktopPlatform () override = default;
  virtual void initialize () override;

private:
  virtual void updateWindowSize () override;
  virtual void mainLoop () override;
};

#endif // __DESKTOPPLATFORM_H__