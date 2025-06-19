#ifndef __GUISTRINGS_H__
#define __GUISTRINGS_H__

#include <string>
#include <string_view>

// Just one hardcoded string for the entire application
// Anothers are going to be loaded from a json file

inline constexpr std::string_view kAboutWebsite
    = R"(Digitalspace.name - is a personal website implemented as a single application written in C++. Using a specialized compiler, the project has been transformed into a format that runs directly in a standard web browser. Instead of traditional web technologies like HTML and JavaScript, it uses WebGL and WebAssembly to deliver a native-like application experience in the browser. The goal is to demonstrate an alternative approach to building interactive web content. The source code is currently not publicly available.

Credits:
- coffee maker
- c++ programming language
- conan, cmake, cpm, make, ninja building tools
- sdl2, fmt, glm, glew, imgui frameworks
- webassembly, emscripten
- opengl/webgl
- shaders by Twitter:@The_ArtOfCode
- llm's claude & chatgpt for lot of help
- Background music by Bedřich Smetana, Vltava (The Moldau) from Má vlast)";

// Function to convert a string_view to a string
inline std::string to_string (std::string_view sv) {
  return std::string (sv);
}

#endif // __GUISTRINGS_H__