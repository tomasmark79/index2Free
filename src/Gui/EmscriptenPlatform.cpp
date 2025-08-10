#include "EmscriptenPlatform.hpp"

#include "Logger/Logger.hpp"
#include "../Shaders/ShaderConvertor.hpp" // Pro ShaderTarget enum

#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>
#include <cctype>

EM_JS (float, getDevicePixelRatio, (), { return window.devicePixelRatio || 1.0; });
EM_JS (int, getScreenWidth, (), { return screen.width; });
EM_JS (int, getScreenHeight, (), { return screen.height; });
EM_JS (int, getWindowWidth, (), { return window.innerWidth; });
EM_JS (int, getWindowHeight, (), { return window.innerHeight; });

EM_JS (bool, isTouchDevice, (),
       { return 'ontouchstart' in window || navigator.maxTouchPoints > 0; });
EM_JS (bool, isDocumentFullscreen, (), {
  return !!(document.fullscreenElement || document.webkitFullscreenElement
            || document.mozFullScreenElement || document.msFullscreenElement);
});

// clang-format off
EM_JS (bool, isMobileDevice, (), {
  return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
});
// clang-format on

EM_JS (int, detectWebGLVersionJS, (), {
  // Create a temporary canvas for testing
  var canvas = document.createElement ('canvas');

  // WebGL2 context attributes for hardware acceleration
  var webgl2Attributes = {
    alpha: false,
    depth: true,
    stencil: false,
    antialias: true,
    premultipliedAlpha: false,
    preserveDrawingBuffer: false,
    powerPreference: "high-performance",  // Force high-performance GPU
    failIfMajorPerformanceCaveat: false,  // Don't fail if software rendering
    desynchronized: false
  };

  // WebGL1 context attributes for hardware acceleration
  var webgl1Attributes = {
    alpha: false,
    depth: true,
    stencil: false,
    antialias: true,
    premultipliedAlpha: false,
    preserveDrawingBuffer: false,
    powerPreference: "high-performance",  // Force high-performance GPU
    failIfMajorPerformanceCaveat: false   // Don't fail if software rendering
  };

  // Try WebGL2 first with hardware acceleration attributes
  var gl2 = canvas.getContext ('webgl2', webgl2Attributes);
  if (gl2) {
    console.log ('🟢 WebGL 2.0 detected and available');
    console.log ('WebGL 2.0 Version:', gl2.getParameter (gl2.VERSION));
    console.log ('WebGL 2.0 Renderer:', gl2.getParameter (gl2.RENDERER));
    console.log ('WebGL 2.0 Vendor:', gl2.getParameter (gl2.VENDOR));
    console.log ('WebGL 2.0 GLSL Version:', gl2.getParameter (gl2.SHADING_LANGUAGE_VERSION));
    
    // Check if hardware acceleration is enabled
    var debugInfo = gl2.getExtension('WEBGL_debug_renderer_info');
    if (debugInfo) {
      var vendor = gl2.getParameter(debugInfo.UNMASKED_VENDOR_WEBGL);
      var renderer = gl2.getParameter(debugInfo.UNMASKED_RENDERER_WEBGL);
      console.log ('WebGL 2.0 Unmasked Vendor:', vendor);
      console.log ('WebGL 2.0 Unmasked Renderer:', renderer);
      
      // Check for software rendering indicators
      if (renderer && (renderer.toLowerCase().includes('software') || 
                      renderer.toLowerCase().includes('llvmpipe') ||
                      renderer.toLowerCase().includes('mesa'))) {
        console.warn ('⚠️ WebGL 2.0 using software rendering');
      } else {
        console.log ('✅ WebGL 2.0 using hardware acceleration');
      }
    }
    return 2;
  }

  // Fall back to WebGL1 with hardware acceleration attributes
  var gl1 = canvas.getContext ('webgl', webgl1Attributes) || canvas.getContext ('experimental-webgl', webgl1Attributes);
  if (gl1) {
    console.log ('🟡 WebGL 1.0 detected (WebGL 2.0 not available)');
    console.log ('WebGL 1.0 Version:', gl1.getParameter (gl1.VERSION));
    console.log ('WebGL 1.0 Renderer:', gl1.getParameter (gl1.RENDERER));
    console.log ('WebGL 1.0 Vendor:', gl1.getParameter (gl1.VENDOR));
    console.log ('WebGL 1.0 GLSL Version:', gl1.getParameter (gl1.SHADING_LANGUAGE_VERSION));
    
    // Check if hardware acceleration is enabled
    var debugInfo = gl1.getExtension('WEBGL_debug_renderer_info');
    if (debugInfo) {
      var vendor = gl1.getParameter(debugInfo.UNMASKED_VENDOR_WEBGL);
      var renderer = gl1.getParameter(debugInfo.UNMASKED_RENDERER_WEBGL);
      console.log ('WebGL 1.0 Unmasked Vendor:', vendor);
      console.log ('WebGL 1.0 Unmasked Renderer:', renderer);
    }
    return 1;
  }

  console.error ('🔴 No WebGL support detected!');
  return 0;
});

// Function to get display information
EM_JS (void, getDisplayInfo, (float* devicePixelRatio, int* windowWidth, int* windowHeight), {
  setValue (devicePixelRatio, window.devicePixelRatio || 1.0, 'float');
  setValue (windowWidth, window.innerWidth, 'i32');
  setValue (windowHeight, window.innerHeight, 'i32');
});

struct EmscriptenDisplayInfo {
  float devicePixelRatio;
  int windowWidth, windowHeight;
  bool isScaled () const {
    return devicePixelRatio > 1.0f;
  }
  float getEffectiveScale () const {
    return isScaled () ? devicePixelRatio : 1.0f;
  }
  void update () {
    getDisplayInfo (&devicePixelRatio, &windowWidth, &windowHeight);
  }
};

WebGLVersion EmscriptenPlatform::detectWebGLVersion () {
  int version = detectWebGLVersionJS ();
  switch (version) {
  case 2:
    return WebGLVersion::WEBGL2;
  case 1:
    return WebGLVersion::WEBGL1;
  default:
    LOG_E_STREAM << "No WebGL support detected! Falling back to WebGL1." << std::endl;
    return WebGLVersion::WEBGL1;
  }
}

void EmscriptenPlatform::logWebGLInfo (WebGLVersion version) {
  switch (version) {
  case WebGLVersion::WEBGL2:
    LOG_I_STREAM << "🚀 Using WebGL 2.0 (OpenGL ES 3.0)" << std::endl;
    break;
  case WebGLVersion::WEBGL1:
    LOG_I_STREAM << "📟 Using WebGL 1.0 (OpenGL ES 2.0)" << std::endl;
    break;
  }
}

void EmscriptenPlatform::decideOpenGLVersionForEmscripten () {
  if (currentWebGLVersion_ == WebGLVersion::WEBGL2) {
    // GL ES 3.0 + GLSL 300 es (WebGL 2.0)
    glsl_version_ = "#version 300 es";
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
    
    // Additional attributes for hardware acceleration
    SDL_GL_SetAttribute (SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute (SDL_GL_ALPHA_SIZE, 0);
  } else {
    // GL ES 2.0 + GLSL 100 (WebGL 1.0)
    glsl_version_ = "#version 100";
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
    
    // Additional attributes for hardware acceleration
    SDL_GL_SetAttribute (SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute (SDL_GL_ALPHA_SIZE, 0);
  }
}

void EmscriptenPlatform::initialize () {

  currentWebGLVersion_ = detectWebGLVersion ();
  logWebGLInfo (currentWebGLVersion_);

  const bool isInfiniteLoop = true; // Emscripten main loop runs indefinitely
  createSDL2Window ("Emscripten SDL2 Window", windowWidth_, windowHeight_);
  
  // IMPORTANT: Set GLSL version BEFORE creating any OpenGL context or initializing ImGui
  decideOpenGLVersion();
  
  // Use native Emscripten WebGL API for better hardware acceleration support
  createEmscriptenWebGLContext();
  
  // Check hardware acceleration after WebGL context is created
  checkHardwareAcceleration();
  
  // Run GPU performance test
  performGPUPerformanceTest();
  
  setupQuad ();
  setupShaders ();
  
  // Log GLSL version before ImGui initialization
  LOG_I_STREAM << "🔧 Using GLSL version for ImGui: " << glsl_version_ << std::endl;
  
  initializeImGui ();
  updateWindowSize ();
  scaleImGui (this->userScaleFactor);
  initInputHandlerCallbacks ();
  // Set up main loop with error handling for Chrome compatibility
  try {
    emscripten_set_main_loop_arg (
        [] (void* userData) {
          EmscriptenPlatform* platform = static_cast<EmscriptenPlatform*> (userData);
          platform->mainLoop ();
        },
        this, 0, isInfiniteLoop);
  } catch (...) {
    LOG_W_STREAM << "⚠️ Main loop setup failed, trying alternative approach" << std::endl;
    // Alternative setup without timing constraints
    emscripten_set_main_loop_arg (
        [] (void* userData) {
          EmscriptenPlatform* platform = static_cast<EmscriptenPlatform*> (userData);
          platform->mainLoop ();
        },
        this, -1, isInfiniteLoop);
  }
}

int EmscriptenPlatform::getShaderTarget () {
  if (currentWebGLVersion_ == WebGLVersion::WEBGL2) {
    return static_cast<int> (ShaderTarget::WebGL2);
  } else {
    return static_cast<int> (ShaderTarget::WebGL1);
  }
}

// Add JavaScript function to perform GPU performance test
EM_JS (void, performGPUPerformanceTest, (), {
  console.log('🏃 Running GPU performance test...');
  
  // Create test canvas
  const testCanvas = document.createElement('canvas');
  testCanvas.width = 512;
  testCanvas.height = 512;
  
  const attrs = {
    alpha: false,
    depth: true,
    stencil: false,
    antialias: true,
    premultipliedAlpha: false,
    preserveDrawingBuffer: false,
    powerPreference: "high-performance",
    failIfMajorPerformanceCaveat: false
  };
  
  const gl = testCanvas.getContext('webgl2', attrs) || testCanvas.getContext('webgl', attrs);
  if (!gl) {
    console.error('❌ Could not create WebGL context for performance test');
    return;
  }
  
  // Simple performance test - draw many triangles
  const vertexShader = gl.createShader(gl.VERTEX_SHADER);
  const fragmentShader = gl.createShader(gl.FRAGMENT_SHADER);
  
  const vertSource = gl.getParameter(gl.VERSION).includes('WebGL 2.0') ? 
    '#version 300 es\nin vec2 position; void main() { gl_Position = vec4(position, 0.0, 1.0); }' :
    'attribute vec2 position; void main() { gl_Position = vec4(position, 0.0, 1.0); }';
    
  const fragSource = gl.getParameter(gl.VERSION).includes('WebGL 2.0') ?
    '#version 300 es\nprecision mediump float; out vec4 fragColor; void main() { fragColor = vec4(1.0, 0.0, 0.0, 1.0); }' :
    'precision mediump float; void main() { gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); }';
  
  gl.shaderSource(vertexShader, vertSource);
  gl.shaderSource(fragmentShader, fragSource);
  gl.compileShader(vertexShader);
  gl.compileShader(fragmentShader);
  
  if (!gl.getShaderParameter(vertexShader, gl.COMPILE_STATUS) || 
      !gl.getShaderParameter(fragmentShader, gl.COMPILE_STATUS)) {
    console.warn('⚠️ Could not compile test shaders');
    return;
  }
  
  const program = gl.createProgram();
  gl.attachShader(program, vertexShader);
  gl.attachShader(program, fragmentShader);
  gl.linkProgram(program);
  
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    console.warn('⚠️ Could not link test program');
    return;
  }
  
  // Performance measurement
  const startTime = performance.now();
  gl.useProgram(program);
  
  // Draw multiple times to stress test
  for (let i = 0; i < 1000; i++) {
    gl.drawArrays(gl.TRIANGLES, 0, 3);
  }
  gl.finish(); // Wait for GPU to complete
  
  const endTime = performance.now();
  const duration = endTime - startTime;
  
  console.log('⚡ GPU Performance Test Results:');
  console.log('  - Duration: ' + duration.toFixed(2) + ' ms');
  console.log('  - Draw calls: 1000');
  console.log('  - Avg per call: ' + (duration / 1000).toFixed(3) + ' ms');
  
  if (duration < 50) {
    console.log('🚀 Excellent GPU performance - likely using hardware acceleration');
  } else if (duration < 200) {
    console.log('⚡ Good GPU performance - hardware acceleration probable');
  } else {
    console.log('🐌 Slow GPU performance - may be using software rendering');
  }
  
  // Memory info if available
  const ext = gl.getExtension('WEBGL_debug_renderer_info');
  if (ext) {
    const renderer = gl.getParameter(ext.UNMASKED_RENDERER_WEBGL);
    console.log('🔍 Renderer during test: ' + renderer);
  }
});
  // Check OpenGL renderer and vendor to detect software rendering
  const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
  const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
  const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
  
  LOG_I_STREAM << "🔍 OpenGL Vendor: " << (vendor ? vendor : "Unknown") << std::endl;
  LOG_I_STREAM << "🔍 OpenGL Renderer: " << (renderer ? renderer : "Unknown") << std::endl;
  LOG_I_STREAM << "🔍 OpenGL Version: " << (version ? version : "Unknown") << std::endl;
  
  if (renderer) {
    std::string rendererStr = std::string(renderer);
    std::transform(rendererStr.begin(), rendererStr.end(), rendererStr.begin(), ::tolower);
    
    // In WebGL/Emscripten, "WebKit WebGL" is normal and doesn't indicate software rendering
    // Only warn for actual software rendering indicators
    if (rendererStr.find("software") != std::string::npos ||
        rendererStr.find("llvmpipe") != std::string::npos ||
        rendererStr.find("mesa software") != std::string::npos ||
        rendererStr.find("swiftshader") != std::string::npos) {
      LOG_W_STREAM << "⚠️ Software rendering detected. Hardware acceleration may not be available." << std::endl;
      LOG_W_STREAM << "💡 Try enabling hardware acceleration in Chrome settings or updating graphics drivers." << std::endl;
    } else {
      // WebKit WebGL is the normal WebGL wrapper - hardware acceleration status is determined by JavaScript
      LOG_I_STREAM << "✅ WebGL context created successfully. Hardware acceleration status determined by browser." << std::endl;
    }
  }
}

void EmscriptenPlatform::updateWindowSize () {
  int width, height;
  SDL_GL_GetDrawableSize (window_, &width, &height);
  if (width != windowWidth_ || height != windowHeight_) {
    windowWidth_ = width;
    windowHeight_ = height;
    devicePixelRatio_ = getDevicePixelRatio (); // JavaScript function to get device pixel ratio
    io_->DisplaySize = ImVec2 ((float)windowWidth_, (float)windowHeight_);
    io_->DisplayFramebufferScale = ImVec2 (devicePixelRatio_, devicePixelRatio_);
  }
}

void EmscriptenPlatform::mainLoop () {
  // For Emscripten: Single iteration - called by emscripten_set_main_loop_arg
  this->updateWindowSize ();

  SDL_Event event;
  while (SDL_PollEvent (&event)) {
    ImGui_ImplSDL2_ProcessEvent (&event);
    bool done = inputHandler.processEvent (event); // own event processing
    if (done) {
      emscripten_cancel_main_loop (); // Stop the main loop in Emscripten
      return;
    }
  }

  ImGui_ImplOpenGL3_NewFrame ();
  ImGui_ImplSDL2_NewFrame ();
  ImGui::NewFrame ();

  this->buildImguiContent ();

  ImGui::Render ();
  glViewport (0, 0, windowWidth_, windowHeight_);
  glClearColor (0.45f, 0.55f, 0.60f, 1.00f);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Render background shader - cumulative time using SDL_GetTicks for better precision
  static float totalTime = 0.0f;
  static Uint32 lastTime = SDL_GetTicks ();
  Uint32 currentTime = SDL_GetTicks ();

  float deltaTime = (currentTime - lastTime) / 1000.0f;
  totalTime += deltaTime;
  lastTime = currentTime;

  renderBackground (totalTime); // Pass cumulative time, not delta
  ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData ());
  SDL_GL_SwapWindow (window_);
}

void EmscriptenPlatform::createEmscriptenWebGLContext() {
#ifdef __EMSCRIPTEN__
  LOG_I_STREAM << "🔧 Creating Emscripten WebGL context..." << std::endl;
  
  // Initialize WebGL context attributes with hardware acceleration preferences
  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  
  // Configure for hardware acceleration
  attrs.alpha = false;                    // Disable alpha for better performance
  attrs.depth = true;                     // Enable depth buffer
  attrs.stencil = false;                  // Disable stencil buffer for better performance
  attrs.antialias = true;                 // Enable antialiasing
  attrs.premultipliedAlpha = false;       // Better performance
  attrs.preserveDrawingBuffer = false;    // Better performance
  attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;  // Force high-performance GPU
  attrs.failIfMajorPerformanceCaveat = false;  // Don't fail if only software rendering available
  attrs.majorVersion = (currentWebGLVersion_ == WebGLVersion::WEBGL2) ? 2 : 1;
  attrs.minorVersion = 0;
  attrs.enableExtensionsByDefault = true;  // Auto-enable compatible extensions
  
  LOG_I_STREAM << "🔧 WebGL context attributes:" << std::endl;
  LOG_I_STREAM << "  - majorVersion: " << attrs.majorVersion << std::endl;
  LOG_I_STREAM << "  - powerPreference: " << attrs.powerPreference << std::endl;
  LOG_I_STREAM << "  - failIfMajorPerformanceCaveat: " << (attrs.failIfMajorPerformanceCaveat ? "true" : "false") << std::endl;
  
  // Create WebGL context on the canvas
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = emscripten_webgl_create_context("#canvas", &attrs);
  
  if (context > 0) {
    LOG_I_STREAM << "✅ WebGL context handle: " << context << std::endl;
    
    // Make the context current
    EMSCRIPTEN_RESULT result = emscripten_webgl_make_context_current(context);
    if (result == EMSCRIPTEN_RESULT_SUCCESS) {
      LOG_I_STREAM << "✅ Emscripten WebGL context made current successfully" << std::endl;
      
      // Get actual context attributes that were created
      EmscriptenWebGLContextAttributes actualAttrs;
      EMSCRIPTEN_RESULT attrResult = emscripten_webgl_get_context_attributes(context, &actualAttrs);
      if (attrResult == EMSCRIPTEN_RESULT_SUCCESS) {
        LOG_I_STREAM << "🔍 Actual WebGL context attributes:" << std::endl;
        LOG_I_STREAM << "  - majorVersion: " << actualAttrs.majorVersion << std::endl;
        LOG_I_STREAM << "  - powerPreference: " << actualAttrs.powerPreference << std::endl;
        LOG_I_STREAM << "  - failIfMajorPerformanceCaveat: " << (actualAttrs.failIfMajorPerformanceCaveat ? "true" : "false") << std::endl;
      }
      
    } else {
      LOG_E_STREAM << "❌ Failed to make Emscripten WebGL context current, result: " << result << std::endl;
    }
  } else {
    LOG_E_STREAM << "❌ Failed to create Emscripten WebGL context, handle: " << context << std::endl;
  }
#endif
}
