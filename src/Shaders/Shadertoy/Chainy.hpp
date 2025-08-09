#ifndef __CHAINY_H__
#define __CHAINY_H__

const char* fragmentShaderToyChainy = R"(
//  CC BY-NC-SA 4.0
//
// This shader implements a physically correct model for the fog glow around a point light.
// The real fog glow comes mostly from Mie-scattering or similar large particle interactions.
// 1. The volume is assumed to be homogeneous (constant density).
// 2. Only single event scattering is considered.
// 3. No light dimming due to absorption is assumed. (energy conservation)
// 4. The scattering phase-function is isotropic. (all scattering directions are equally likely)
//
// The upper half (above mouse cursor) is the correct analytic result
// The lower half (below mouse cursor) is a cheap approximation
//
// My prior mostly artistic version is here: https://www.shadertoy.com/view/7stfDr
// For another artistic version see: https://www.shadertoy.com/view/3s3GDn
// For more scattering physics see: https://www.shadertoy.com/view/WftSD7
//
// I'm working on a ground truth Monte Carlo version, but it is currently somewhat broken.

// isotropic scattering constant
// #define DENSITY 0.05*pow(length(iMouse)==0.0 ? 1.25 : iMouse.x/iResolution.x+1e-5, 4.)
#define DENSITY 0.02*pow(length(iMouse)==0.0 ? 0.25 : iMouse.x/iResolution.x+1e-5, 2.)

// use a simpler approximation without complex numbers:
#define SIMPLE length(iMouse)!=0.0 && iMouse.y-fragCoord.y>0.

float halo_simple(vec3 pos, vec3 dir, vec3 lightpos, float density) {
    // simple combination of the two limiting cases for high and low density
    // Note, that tuning some scaling values for a better result must be done with different distances!
    vec3 l = lightpos - pos;
    float d = length(l), t = dot(dir, l), b = length(cross(dir, l));
    return mix(1./(d*d), density * (1.5708 + atan(t/b))/b, exp(-d*density));
}

// complex exponential
vec2 cexp(vec2 c) {
    return exp(c.x)*vec2(cos(c.y), sin(c.y));
}
// complex logarithm
vec2 cln(vec2 c) {
    float r = length(c);
    return vec2(log(r), atan(c.y, c.x));
}
// complex multiplication
vec2 cmul(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}
// complex conjugation
vec2 conj(vec2 c) { return vec2(c.x, -c.y); }
// complex division
vec2 cdiv(vec2 a, vec2 b) {
    return cmul(a, conj(b)) / dot(b, b);
}
// complex exponential integral E_1(x)*e^x
vec2 expintr(vec2 x) {
    const vec2 y = vec2(0.577215664902,0); // Euler-Mascheroni constant
    // my approximation of E_1(x)*e^x for complex arguments with low error where E_1(x) is a version of the exponential integral.
    if (length(x + vec2(2.5,0)) < 4.2) {
        // for small |x|, use the taylor series
        vec2 p = vec2(1,0), s = vec2(0);
        float f = -1.;
        for (int n = 1; n <= 12; n++) {
            p = cmul(p, x);
            s += p * f / float(n);
            f /= -float(n + 1);
        }
        return cmul(-y - cln(x) - s, cexp(x));
    }
    else {
        // my approximation for large |x|
        const vec2 ey = vec2(exp(-y.x),0), a1 = vec2(1.36889,0), a2 = vec2(-0.324484,0), b1 = vec2(3.44011,0), b2 = vec2(1.34683,0);
        const float a3 = -0.0587525, b3 = -0.78424;
        x = cdiv(vec2(1,0), x);
        return -y - cln(cdiv(ey + cmul(x, a1 + cmul(x, a2 + x * a3)), vec2(1,0) + cmul(x, b1 + cmul(x, b2 + x * b3))));
    }
}

float halo_analytic(vec3 pos, vec3 dir, vec3 lightpos, float density) {
    // assumption: only one scattering event + energy conservation
    vec3 to_light = lightpos - pos;
    float depth = dot(dir, to_light);
    float b = length(cross(dir, to_light));
    return density/b * expintr(-density * vec2(depth, b)).y;
}

// my own nice color palette
vec3 colors(float i) {
    return max(abs(sin(i*.5+vec3(0,1,2))),0.);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (2.0*fragCoord - iResolution.xy) / iResolution.y * 1.0;
    // perspective view
    vec3 rpos = vec3(50,0,0),
         d = normalize(vec3(0)-rpos),
         up = vec3(0,0,1),
         x = normalize(cross(d,up)),
         y = normalize(cross(x,d)),
         rdir = normalize(1.7*d + mat2x3(x,y)*uv);

    vec3 linear_col = vec3(0.0);
    for (float x = 0.; x < 6.28; x += 0.3) {
        vec3 lightpos = vec3(0,sin(x),cos(x))*(20.+2.*sin(sin(x)*5.*iTime+iTime));
        lightpos.x += 20.*sin(2.*x + iTime);

        float density = DENSITY, fog;
        // add direct unscattered light
        float miss_dist = length(cross(rpos - lightpos, rdir));
        float dist = distance(lightpos, rpos);
        float point = exp(-dist*density);
        // add scattered light.
        // pointlight size is handled, but not physically correct.
        if (SIMPLE)
            fog = min(halo_simple(rpos, rdir, lightpos, density), point+(1.+point)/(dist * dist));
        else
            fog = min(halo_analytic(rpos, rdir, lightpos, density), point+(1.+point)/(dist * dist));
        point *= smoothstep(0., -0.1, miss_dist - 0.4);
        linear_col += (point + fog) * colors(x) * 20.; // color the light (not the absorption)
    }
    vec3 col = pow(linear_col, vec3(1./2.2)); // gamma correction
    // tonemapping to get desaturated color for large intensities (human vision)
    // this only affects clipping colors and doesn't change the actual brightness value.
    float value = max(max(col.x, col.y), col.z);
    vec3 tonemapped = col + max(0., value-1.);
    fragColor = vec4(tonemapped, 1.0);
}
)";
#endif // __CHAINY_H__