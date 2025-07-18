#ifndef __FRAGMENT_DEF_H__
#define __FRAGMENT_DEF_H__

// WebGL1 shaders (OpenGL ES 2.0)
const char* fragmentShader200 = R"(
precision highp float;

uniform vec2 iResolution;
uniform float iTime;
uniform float iTimeDelta;
uniform int iFrame;
uniform vec4 iMouse;

float opSmoothUnion( float d1, float d2, float k )
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h);
}

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
} 

float map(vec3 p)
{
    float d = 2.0;
    for (int i = 0; i < 16; i++) {
        float fi = float(i);
        float time = iTime * (fract(fi * 412.531 + 0.513) - 0.5) * 2.0;
        d = opSmoothUnion(
            sdSphere(p + sin(time + fi * vec3(52.5126, 64.62744, 632.25)) * vec3(2.0, 2.0, 0.8), mix(0.5, 1.0, fract(fi * 412.531 + 0.5124))),
            d,
            0.4
        );
    }
    return d;
}

vec3 calcNormal( vec3 p )
{
    float h = 1e-5;
    vec2 k = vec2(1,-1);
    return normalize( k.xyy*map( p + k.xyy*h ) + 
                      k.yyx*map( p + k.yyx*h ) + 
                      k.yxy*map( p + k.yxy*h ) + 
                      k.xxx*map( p + k.xxx*h ) );
}

void mainImage( vec4 outColor, vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    
    // screen size is 6m x 6m
    vec3 rayOri = vec3((uv - 0.5) * vec2(iResolution.x/iResolution.y, 1.0) * 6.0, 3.0);
    vec3 rayDir = vec3(0.0, 0.0, -1.0);
    
    float depth = 0.0;
    vec3 p;
    
    for(int i = 0; i < 64; i++) {
        p = rayOri + rayDir * depth;
        float dist = map(p);
        depth += dist;
        if (dist < 1e-6) {
            break;
        }
    }
    
    depth = min(6.0, depth);
    vec3 n = calcNormal(p);
    float b = max(0.0, dot(n, vec3(0.577)));
    vec3 col = (0.5 + 0.5 * cos((b + iTime * 3.0) + uv.xyx * 2.0 + vec3(0,2,4))) * (0.85 + b * 0.35);
    col *= exp( -depth * 0.15 );
    
    // maximum thickness is 2m in alpha channel
    gl_FragColor = vec4(col, 1.0 - (depth - 0.5) / 2.0);
}

void main()
{
    mainImage(gl_FragColor, gl_FragCoord.xy);
}
)";

// WebGL2 shaders (OpenGL ES 3.0)
const char* fragmentShader300 = R"(#version 300 es
precision highp float;

uniform vec2 iResolution;
uniform float iTime;
uniform float iTimeDelta;
uniform int iFrame;
uniform vec4 iMouse;

out vec4 fragColor;

float opSmoothUnion( float d1, float d2, float k )
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h);
}

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
} 

float map(vec3 p)
{
    float d = 2.0;
    for (int i = 0; i < 16; i++) {
        float fi = float(i);
        float time = iTime * (fract(fi * 412.531 + 0.513) - 0.5) * 2.0;
        d = opSmoothUnion(
            sdSphere(p + sin(time + fi * vec3(52.5126, 64.62744, 632.25)) * vec3(2.0, 2.0, 0.8), mix(0.5, 1.0, fract(fi * 412.531 + 0.5124))),
            d,
            0.4
        );
    }
    return d;
}

vec3 calcNormal( in vec3 p )
{
    const float h = 1e-5;
    const vec2 k = vec2(1,-1);
    return normalize( k.xyy*map( p + k.xyy*h ) + 
                      k.yyx*map( p + k.yyx*h ) + 
                      k.yxy*map( p + k.yxy*h ) + 
                      k.xxx*map( p + k.xxx*h ) );
}

void mainImage( out vec4 outColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    
    // screen size is 6m x 6m
    vec3 rayOri = vec3((uv - 0.5) * vec2(iResolution.x/iResolution.y, 1.0) * 6.0, 3.0);
    vec3 rayDir = vec3(0.0, 0.0, -1.0);
    
    float depth = 0.0;
    vec3 p;
    
    for(int i = 0; i < 64; i++) {
        p = rayOri + rayDir * depth;
        float dist = map(p);
        depth += dist;
        if (dist < 1e-6) {
            break;
        }
    }
    
    depth = min(6.0, depth);
    vec3 n = calcNormal(p);
    float b = max(0.0, dot(n, vec3(0.577)));
    vec3 col = (0.5 + 0.5 * cos((b + iTime * 3.0) + uv.xyx * 2.0 + vec3(0,2,4))) * (0.85 + b * 0.35);
    col *= exp( -depth * 0.15 );
    
    // maximum thickness is 2m in alpha channel
    outColor = vec4(col, 1.0 - (depth - 0.5) / 2.0);
}

void main()
{
    mainImage(fragColor, gl_FragCoord.xy);
}
)";

// Desktop OpenGL shaders
const char* fragmentShader330 = R"(#version 330 core

uniform vec2 iResolution;
uniform float iTime;
uniform float iTimeDelta;
uniform int iFrame;
uniform vec4 iMouse;

out vec4 fragColor;

float opSmoothUnion( float d1, float d2, float k )
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h);
}

float sdSphere( vec3 p, float s )
{
  return length(p)-s;
} 

float map(vec3 p)
{
    float d = 2.0;
    for (int i = 0; i < 16; i++) {
        float fi = float(i);
        float time = iTime * (fract(fi * 412.531 + 0.513) - 0.5) * 2.0;
        d = opSmoothUnion(
            sdSphere(p + sin(time + fi * vec3(52.5126, 64.62744, 632.25)) * vec3(2.0, 2.0, 0.8), mix(0.5, 1.0, fract(fi * 412.531 + 0.5124))),
            d,
            0.4
        );
    }
    return d;
}

vec3 calcNormal( in vec3 p )
{
    const float h = 1e-5;
    const vec2 k = vec2(1,-1);
    return normalize( k.xyy*map( p + k.xyy*h ) + 
                      k.yyx*map( p + k.yyx*h ) + 
                      k.yxy*map( p + k.yxy*h ) + 
                      k.xxx*map( p + k.xxx*h ) );
}

void mainImage( out vec4 outColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    
    // screen size is 6m x 6m
    vec3 rayOri = vec3((uv - 0.5) * vec2(iResolution.x/iResolution.y, 1.0) * 6.0, 3.0);
    vec3 rayDir = vec3(0.0, 0.0, -1.0);
    
    float depth = 0.0;
    vec3 p;
    
    for(int i = 0; i < 64; i++) {
        p = rayOri + rayDir * depth;
        float dist = map(p);
        depth += dist;
        if (dist < 1e-6) {
            break;
        }
    }
    
    depth = min(6.0, depth);
    vec3 n = calcNormal(p);
    float b = max(0.0, dot(n, vec3(0.577)));
    vec3 col = (0.5 + 0.5 * cos((b + iTime * 3.0) + uv.xyx * 2.0 + vec3(0,2,4))) * (0.85 + b * 0.35);
    col *= exp( -depth * 0.15 );
    
    // maximum thickness is 2m in alpha channel
    outColor = vec4(col, 1.0 - (depth - 0.5) / 2.0);
}

void main()
{
    mainImage(fragColor, gl_FragCoord.xy);
}
)";

#endif // __FRAGMENT_DEF_H__