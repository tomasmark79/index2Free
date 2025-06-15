#ifndef BACKGROUNDMAGIC_HPP
#define BACKGROUNDMAGIC_HPP

#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <Utils/Utils.hpp>

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <filesystem>

#if defined(IMGUI_IMPL_OPENGL_ES2) || defined(IMGUI_IMPL_OPENGL_ES3)
  #include <SDL_opengles2.h>
#else
  #include <GL/glew.h>
#endif

class BackgroundMagic {
public:
  BackgroundMagic ();
  ~BackgroundMagic ();

private:
};

namespace TextureLoader {
  bool LoadTextureFromMemory (const void* data, size_t data_size, SDL_Renderer* renderer,
                              SDL_Texture** out_texture, int* out_width, int* out_height);
  bool LoadTextureFromFile (const std::filesystem::path& file_path, SDL_Renderer* renderer,
                            SDL_Texture** out_texture, int* out_width, int* out_height);
} // namespace TextureLoader

#endif