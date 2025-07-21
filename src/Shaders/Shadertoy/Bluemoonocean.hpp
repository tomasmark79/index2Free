#ifndef __BLUE_MONOCEAN_H__
#define __BLUE_MONOCEAN_H__

const char* fragmentShaderToyBluemoonocean = R"(
/*
    -9 chars from iapafoto :D
*/

void mainImage(out vec4 o, vec2 u) {
    float i,d,s,t=iTime;
    vec3  p = iResolution;    
    u = (u-p.xy/2.)/p.y;
    for(o*=i; i++<1e2; ) {
        p = vec3(u* d,d+t);
        for (s = .15; s < 1.;
            p += cos(t+p.yzx*.6)*sin(p.z*.1)*.2,
            p.y += sin(t+p.x)*.03,
            p += abs(dot(sin(p * s * 24.), p-p+.01)) / s,
            s *= 1.5);
        d += s = .03 + abs(2.+p.y)*.3;
        o += vec4(1,2,4,0) / s;
    }
    o = tanh(o / 7e3 / dot(u-=.35,u));
}

/*
    See here for how to do a tiny raymarch loop and translucency:
        https://www.shadertoy.com/view/wXjSRt
        https://www.shadertoy.com/view/wXSXzV
*/




/*
    -2 chars from iapafoto :D

void mainImage(out vec4 o, vec2 u) {
    float i,d,s,t=iTime;
    vec3  p = iResolution;    
    u = (u-p.xy/2.)/p.y;
    for(o*=i; i++<1e2; ) {
        p = vec3(u,1) * d;
        p.z += t;
        for (s = .15; s < 1.;
            p += cos(t+p.yzx*.6)*sin(p.z*.1)*.2,
            p.y += sin(t+p.x)*.03,
            p += abs(dot(sin(p * s * 24.), p-p+.01)) / s,
            s *= 1.5);
        d += s = .03 + abs(2.+p.y)*.3;
        o += vec4(1,2,4,0) / s;
    }
    u -= .35;
    o = tanh(o / 7e3 / dot(u,u));
}
*/
)";

#endif // __BLUE_MONOCEAN_H__