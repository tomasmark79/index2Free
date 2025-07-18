#ifndef __FRAGMENT_DEF_H__
#define __FRAGMENT_DEF_H__

// WebGL1 shaders (OpenGL ES 2.0)
const char* fragmentShader200 = R"(
precision mediump float;
uniform float iTime;
uniform vec2 iResolution;
varying vec2 uv;

void main() {
    vec2 coord = uv * 2.0 - 1.0;
    coord.x *= iResolution.x / iResolution.y;
    
    float dist = length(coord);
    float angle = atan(coord.y, coord.x);
    
    // Animated spiral pattern
    float spiral = sin(dist * 8.0 - iTime * 2.0 + angle * 3.0) * 0.5 + 0.5;
    
    // Color waves
    vec3 color1 = vec3(0.8, 0.2, 0.7); // Purple
    vec3 color2 = vec3(0.2, 0.8, 0.9); // Cyan
    vec3 color3 = vec3(1.0, 0.6, 0.1); // Orange
    
    float wave1 = sin(iTime * 1.5 + dist * 5.0) * 0.5 + 0.5;
    float wave2 = cos(iTime * 2.0 + angle * 2.0) * 0.5 + 0.5;
    
    vec3 finalColor = mix(color1, color2, wave1);
    finalColor = mix(finalColor, color3, wave2 * spiral);
    
    // Add some sparkle
    float sparkle = smoothstep(0.8, 1.0, spiral) * (sin(iTime * 10.0) * 0.5 + 0.5);
    finalColor += vec3(sparkle);
    
    gl_FragColor = vec4(finalColor, 1.0);
}
)";

// WebGL2 shaders (OpenGL ES 3.0)
const char* fragmentShader300 = R"(#version 300 es
precision mediump float;
uniform float iTime;
uniform vec2 iResolution;
in vec2 uv;

out vec4 fragColor;

void main() {
    vec2 coord = uv * 2.0 - 1.0;
    coord.x *= iResolution.x / iResolution.y;
    
    float dist = length(coord);
    float angle = atan(coord.y, coord.x);
    
    // Animated spiral pattern
    float spiral = sin(dist * 8.0 - iTime * 2.0 + angle * 3.0) * 0.5 + 0.5;
    
    // Color waves
    vec3 color1 = vec3(0.8, 0.2, 0.7); // Purple
    vec3 color2 = vec3(0.2, 0.8, 0.9); // Cyan
    vec3 color3 = vec3(1.0, 0.6, 0.1); // Orange
    
    float wave1 = sin(iTime * 1.5 + dist * 5.0) * 0.5 + 0.5;
    float wave2 = cos(iTime * 2.0 + angle * 2.0) * 0.5 + 0.5;
    
    vec3 finalColor = mix(color1, color2, wave1);
    finalColor = mix(finalColor, color3, wave2 * spiral);
    
    // Add some sparkle
    float sparkle = smoothstep(0.8, 1.0, spiral) * (sin(iTime * 10.0) * 0.5 + 0.5);
    finalColor += vec3(sparkle);
    
    fragColor = vec4(finalColor, 1.0);
}
)";

// Desktop OpenGL shaders
const char* fragmentShader330 = R"(#version 330 core
uniform float iTime;
uniform vec2 iResolution;
in vec2 uv;

out vec4 fragColor;

void main() {
    vec2 coord = uv * 2.0 - 1.0;
    coord.x *= iResolution.x / iResolution.y;
    
    float dist = length(coord);
    float angle = atan(coord.y, coord.x);
    
    // Animated spiral pattern
    float spiral = sin(dist * 8.0 - iTime * 2.0 + angle * 3.0) * 0.5 + 0.5;
    
    // Color waves
    vec3 color1 = vec3(0.8, 0.2, 0.7); // Purple
    vec3 color2 = vec3(0.2, 0.8, 0.9); // Cyan
    vec3 color3 = vec3(1.0, 0.6, 0.1); // Orange
    
    float wave1 = sin(iTime * 1.5 + dist * 5.0) * 0.5 + 0.5;
    float wave2 = cos(iTime * 2.0 + angle * 2.0) * 0.5 + 0.5;
    
    vec3 finalColor = mix(color1, color2, wave1);
    finalColor = mix(finalColor, color3, wave2 * spiral);
    
    // Add some sparkle
    float sparkle = smoothstep(0.8, 1.0, spiral) * (sin(iTime * 10.0) * 0.5 + 0.5);
    finalColor += vec3(sparkle);
    
    fragColor = vec4(finalColor, 1.0);
}
)";

#endif // __FRAGMENT_DEF_H__