#ifndef __ANOTHERCUBE_H__
#define __ANOTHERCUBE_H__

const char* fragmentShaderToyAnothercube = R"(
// CC0: Just another cube
//  Glowtracers are great for compact coding but I wanted to see how much 
//  I could squeeze a more normal raymarcher in terms of characters used

// Twigl: https://twigl.app?ol=true&ss=-OVhSDKtT98UUMFISv-9

// == Globals ==
// Single-letter variable names are used to save characters (code golfing).
mat2 R;    // A 2D rotation matrix, calculated once per frame in mainImage and used by D.
float d    // Stores the current distance to the scene from the ray's position.
    , z    // Stores the total distance traveled along the ray.
    , G=9. // "Glow" variable. Tracks the closest the ray comes to the object (for volumetric glow effect).
    ;
// == Distance Function (SDF - Signed Distance Field) ==
// This function calculates the shortest distance from any given point 'p' to the scene geometry.
// A positive result means the point is outside the object, negative is inside, zero is on the surface.
// This is the core of "raymarching", as it tells us the largest safe step we can take along a ray.
float D(vec3 p) {
  // Apply two rotations to the point's coordinates. This twists the space the object
  // exists in, making the simple cube shape appear more complex and animated.
  p.xy *= R;
  p.xz *= R;

  // Create a higher-frequency version of the coordinate for detailed surface patterns.
  vec3 P = 123.*p;

  // This creates a volumetric glow effect by tracking the minimum distance
  // to either the existing glow value or a glowing shell around the object.
  G = min(
      G
      // The glowing rings
    , max(
        abs(length(p)-.6)
        // The main object distance calculation:
        // 1. A superquadric (rounded cube-like shape) created by the power formula.
        // The expression `pow(dot(p=p*p*p*p,p),.125)` creates a cube with rounded edges.
        // The `- .5` defines the object's size.
      , d = pow(dot(p=p*p*p*p,p),.125) - .5
        // 2. Surface detail subtraction. Creates small surface variations
        // using high-frequency sine waves for more appealing reflections
        - pow(1.+sin(P.x)*sin(P.y)*sin(P.z),8.)/1e5
      )
    );

  return d;
}

// == Main Render Function ==
// This function is called for every pixel on the screen to determine its color.
// 'o' is the final output color (rgba). 'C' is the input pixel coordinate (xy).
void mainImage(out vec4 o, vec2 C) {
  // Single-letter variable names are used to save characters (code golfing).
  vec3  p  // The current point in 3D space along the ray.
      , O  // Multi-purpose vector: color accumulator, then normal vector, then color again.
      , r=iResolution  // 'r' holds screen resolution, later re-used for epsilon vector and reflection.
        // 'I' is the Ray Direction vector. It's calculated once per pixel.
        // This converts the 2D screen coordinate 'C' into a 3D direction, creating the camera perspective.
      , I=normalize(vec3(C-.5*r.xy, r.y))
        // Base glow color (bluish tint)
      , B=vec3(1,2,9)/1e3
      ;

  // == Raymarching Loop ==
  // This loop "marches" a ray from the camera out into the scene to find what it hits.
  for(
        // -- Initializer (runs once before the loop) --
        // Calculate the rotation matrix for this frame based on time.
        R = mat2(cos(.3*iTime+vec4(0,11,33,0)))
    // -- Condition --
    // Loop while total ray distance z is less than 9 (maximum view distance).
    ; z<9.
    // -- Advancement --
    // This is the core of raymarching: move forward along the ray by the
    // distance 'd' returned by the distance function. This is the largest "safe" step.
    ; z += d
    ) {
    // Calculate the current position 'p' in world space.
    p = z*I;
    p.z -= 2.; // Move the camera's starting position back to z=-2.

    // Get the shortest distance from our current point 'p' to the scene.
    D(p);

    // -- Hit Condition --
    // If the distance 'd' is very small, we consider it a "hit".
    if (d < 1e-3) {
      // -- Calculate Surface Normal --
      // The normal is a vector pointing perpendicularly out from the surface at the hit point.
      // It's essential for lighting. We calculate it by checking the distance field at
      // tiny offsets around the hit point 'p' (this is a numerical gradient calculation).
      // The variables 'z' and 'r' are re-purposed here to save space.
      for (
            z=0.
          , r=vec3(1e-3,0,0) // 'r' is now a tiny offset vector (epsilon).
        ; ++z<4.             // Loop 3 times for x, y, and z axes.
        ; r = r.zxy          // Swizzle 'r' to get (0, 1e-3, 0), then (0, 0, 1e-3).
        )
          // Calculate one component of the normal using central differences and store it in 'O'.
          O.x = D(p+r)-D(p-r)
          // Swizzle O to move the calculated component and prepare for the next.
        , O = O.zxy
        ;

      // -- Lighting and Shading --
      // 'z' is re-purposed to store the fresnel factor (1 + cos(angle)) for edge brightness
      // `dot(O, I)` calculates how much the surface is facing the camera (negative for outward normal).
      z = 1.+dot(O = normalize(O.xzy),I); // O is normalized to become a proper normal vector.
      
      // 'r' is re-purposed to store the reflection vector.
      r = reflect(I,O);
      
      // Calculate where the reflection ray intersects the floor plane at y=-5.
      p = p+r*(5.-p.y)/abs(r.y);
      
      // Calculate the final color of the hit point.
      O =
          // Multiply by the fresnel factor squared for stronger edge reflections.
          z*z *
          // Use a ternary operator to decide the color based on where the reflection goes.
          (
              // If the reflection vector points upward...
              r.y>0. 
              // ...sample a procedural "sky" with radial gradient and blue tint
            ? 5e2*smoothstep(5., 4., d = sqrt(length((p*p).xz))+1.)*d*B
              // ...otherwise, sample the "floor" with red/purple exponential falloff
            : exp(-2.*length(p.xz))*vec3(1,0,6)
          )
        // Add ambient/rim lighting (brighter on upward-facing surfaces).
        + pow(1.+O.y,5.)*B
        ;

      break; // Hit found, exit the loop.
    }
  }
  // == Tonemapping & Output ==
  // Apply tone mapping to convert HDR colors to displayable LDR range.
  // `tanh` provides smooth compression, and `sqrt` acts like gamma correction.
  // Add glow contribution: smaller G values (closer to glow shell) make brighter pixels.
  o = sqrt(tanh(O+B/G)).xyzx;
}
)";

#endif // __ANOTHERCUBE_H__