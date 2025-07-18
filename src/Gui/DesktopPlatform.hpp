#ifndef __DESKTOPPLATFORM_H__
#define __DESKTOPPLATFORM_H__

#include "PlatformManager.hpp"

class DesktopPlatform : public PlatformManager {

public:
  DesktopPlatform () = default;
  ~DesktopPlatform () override = default;

  virtual void initialize () override;

protected:
  virtual void updateWindowSize () override;
};

#endif // __DESKTOPPLATFORM_H__