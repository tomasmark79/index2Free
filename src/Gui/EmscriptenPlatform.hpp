#ifndef __EMSCRIPTENPLATFORM_H__
#define __EMSCRIPTENPLATFORM_H__

#include "PlatformManager.hpp"
#include "emscripten.h"
#include "emscripten/html5.h"

namespace EmscriptenDisplayInfo {
  struct DisplayInfo {
    float devicePixelRatio;
    int windowWidth, windowHeight;
    bool isScaled () const;
    float getEffectiveScale () const;
    void update ();
  };
}

enum class WebGLVersion { WEBGL1 = 1, WEBGL2 = 2 };

class EmscriptenPlatform : public PlatformManager {
public:
  EmscriptenPlatform () = default;
  ~EmscriptenPlatform () override = default;
  WebGLVersion currentWebGLVersion_ = WebGLVersion::WEBGL1;
  virtual void initialize () override;

private:
  WebGLVersion detectWebGLVersionByJS ();
  virtual void decideOpenGLVersionForEmscripten () override;
  virtual void updateWindowSize () override;
  virtual int getShaderTarget () override;
  virtual void mainLoop () override;
};

#endif // __EMSCRIPTENPLATFORM_H__