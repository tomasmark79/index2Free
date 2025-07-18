#ifndef __EMSCRIPTENPLATFORM_H__
#define __EMSCRIPTENPLATFORM_H__

#include "PlatformManager.hpp"
#include <emscripten.h>
#include <emscripten/html5.h>

class EmscriptenPlatform : public PlatformManager {

public:
  EmscriptenPlatform () = default;
  ~EmscriptenPlatform () override = default;

  float userConfigurableScale_ = 1.0f;
  virtual void initialize () override;

protected:
  virtual void updateWindowSize () override;
  virtual std::string getOverlayContent () override;
};

#endif // __EMSCRIPTENPLATFORM_H__