// Simple WebGL-friendly shader for testing
const char* simpleFragmentShader = R"(
void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    
    // Simple animated pattern
    float time = iTime * 0.5;
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(uv, center);
    
    // Rotating colors
    vec3 col = 0.5 + 0.5 * cos(time + uv.xyx + vec3(0, 2, 4));
    col *= smoothstep(0.0, 0.1, dist) * smoothstep(0.8, 0.3, dist);
    
    // Add some movement
    col += 0.2 * sin(uv.x * 10.0 + time) * sin(uv.y * 10.0 + time);
    
    fragColor = vec4(col, 1.0);
}
)";
