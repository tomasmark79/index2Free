#ifndef __VERTEX_DEF_H__
#define __VERTEX_DEF_H__

// WebGL1 shaders (OpenGL ES 2.0)
const char* vertexShader200 = R"(
precision highp float;
attribute vec2 pos;
varying vec2 uv;
void main() {
   uv = pos * 0.5 + 0.5;
   gl_Position = vec4(pos, 0.0, 1.0);
}
)";

// WebGL2 shaders (OpenGL ES 3.0)
const char* vertexShader300 = R"(#version 300 es
precision highp float;
layout (location = 0) in vec2 pos;
out vec2 uv;
void main() {
   uv = pos * 0.5 + 0.5;
   gl_Position = vec4(pos, 0.0, 1.0);
}
)";

// Desktop OpenGL shaders (OpenGL 3.3)
const char* vertexShader330 = R"(#version 330 core
layout (location = 0) in vec2 pos;
out vec2 uv;
void main() {
   uv = pos * 0.5f + 0.5f;
   gl_Position = vec4(pos, 0.0f, 1.0f);
}
)";

#endif // __VERTEX_DEF_H__