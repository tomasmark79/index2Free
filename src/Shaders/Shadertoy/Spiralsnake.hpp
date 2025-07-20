#ifndef __SYNTHWAVE_H__
#define __SYNTHWAVE_H__

const char* fragmentShaderToyTemplate = R"(
//Uncomment line 73 for just texture and mask
float shader2Mask(vec2 fragCoord, vec2 res, float t) {
    vec2 p;
    float i = 0.0, g = 0.0, d = 2.0;

    // Raymarch spiral inward loop
    for(float j = 0.0; j < 70.0; j++) {
        i += 1.0;
        if(d <= 0.001) break;
        
        p = (fragCoord - 0.5 * res) / res.y * g;
        
        p += vec2(0.3) * mat2(cos(g * 2.0), -sin(g * 2.0), sin(g * 2.0), cos(g * 2.0)); // spiral
        g += d = -(length(p) - 2.0 + g / 9.0) / 2.0; // convergence condition
    }

    // Angular transform and pattern slicing
    p = vec2(atan(p.x+p.x+sin(iTime), p.y), g) * (8.28 )+ t * 2.0;
    p = abs(fract(p + vec2(0.0, 0.5 * ceil(p.x))) - 0.5);

    float angle = atan(p.y, p.x);
    float spiralPattern = sin(8.0 * angle + t);
    float falloff = smoothstep(0.0, 70.0, i); // smooth radial distance mask
    float m = spiralPattern * (1.0 - falloff);      // fade pattern near outer edge



    return clamp(m, 0.0, 1.0);
}
void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = fragCoord / iResolution.xy;
    uv = uv * 2.0 - 1.0;
    uv.x *= iResolution.x / iResolution.y;

    // Center of explosion
    vec2 center = vec2(0.0);
    
    // Time variable
    float t = iTime*.5;

    // Radial direction and distance
    vec2 dir = uv - center;
    float dist = length(dir);
    float angle = atan(dir.y, dir.x);

    // Spiral-like radial distortion
    float burst = sin(angle * 10.0 + t * 5.0) * 0.1;
    float ripple = sin(dist * 20.0 - t * 10.0) * 0.02;

    // Apply distortion
    uv += normalize(dir) * (burst + ripple) * smoothstep(0.0, 1.0, 1.0 - dist);

    //color (rainbow radial gradient for fun)
    float col = sin(10.0 * dist - t * 4.0 + angle) * 0.5 + 0.5;
    vec3 color = vec3(sin(col * 6.283 + 0.0), sin(col * 6.283 + 2.0), sin(col * 6.283 + 4.0));
    
    // Get the displacement vector from iChannel1's red and green channels
    vec2 disp = color.rg;

    // Scale the displacement to control the intensity
    disp = (disp - 0.5) * .1; // Center around 0 and reduce strength

    // Displace the UV coordinates
    vec2 displacedUV = uv + disp.rg;

    // Sample the base texture using the displaced coordinates
    vec4 colord = texture(iChannel0, displacedUV);
    vec4 colordist = mix(colord,vec4(color,1.), -.25);
    
    
    float mask = shader2Mask(fragCoord, iResolution.xy, iTime);
    //colordist = texture(iChannel0,uv); 
    // Output to screen
    fragColor = vec4(colordist*mask);
}
)";

#endif // __SYNTHWAVE_H__