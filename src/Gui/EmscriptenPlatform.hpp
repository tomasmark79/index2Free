#ifndef __EMSCRIPTENPLATFORM_H__
#define __EMSCRIPTENPLATFORM_H__

#include "PlatformManager.hpp"
#include <emscripten.h>
#include <emscripten/html5.h>

enum class WebGLVersion { WEBGL1 = 1, WEBGL2 = 2 };

class EmscriptenPlatform : public PlatformManager {

public:
  EmscriptenPlatform () = default;
  ~EmscriptenPlatform () override = default;

  virtual void initialize () override;

protected:
  virtual void updateWindowSize () override;
  void decideOpenGLVersionForEmscripten ();
  WebGLVersion detectWebGLVersion ();
  void logWebGLInfo (WebGLVersion version);

private:
  WebGLVersion currentWebGLVersion_ = WebGLVersion::WEBGL1;
};

#endif // __EMSCRIPTENPLATFORM_H__