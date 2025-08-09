#ifndef __WEBGL2TEST_H__
#define __WEBGL2TEST_H__

const char* fragmentShaderToyWebGL2Test = R"(
// Simple test shader to verify WebGL2 conversion
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Normalized coordinates (0.0 to 1.0)
    vec2 uv = fragCoord / iResolution.xy;
    
    // Test WebGL2 specific features
    
    // 1. texture() function (should work in WebGL2)
    // Note: This would need a texture sampler in real use
    // vec4 texColor = texture(iChannel0, uv);
    
    // 2. textureSize() function
    // ivec2 texSize = textureSize(iChannel0, 0);
    
    // 3. Standard math functions that should work
    float time = iTime * 0.5;
    vec3 color = vec3(0.0);
    
    // Simple gradient based on UV coordinates
    color.r = uv.x;
    color.g = uv.y;
    color.b = 0.5 + 0.5 * sin(time);
    
    // Add some animation using time
    float wave = sin(uv.x * 10.0 + time) * sin(uv.y * 10.0 + time);
    color += 0.2 * wave;
    
    // Test some math functions
    color = clamp(color, 0.0, 1.0);
    color = mix(color, vec3(1.0, 0.5, 0.2), smoothstep(0.4, 0.6, length(uv - 0.5)));
    
    // Test derivatives (WebGL2 feature)
    vec2 dx = dFdx(uv);
    vec2 dy = dFdy(uv);
    float edge = length(dx) + length(dy);
    color += 0.1 * edge;
    
    fragColor = vec4(color, 1.0);
}
)";

#endif // __WEBGL2TEST_H__
