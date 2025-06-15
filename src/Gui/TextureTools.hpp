#ifndef __TEXTURETOOLS_H__
#define __TEXTURETOOLS_H__

#include <Assets/AssetContext.hpp>
#include <Logger/Logger.hpp>
#include <Utils/Utils.hpp>
#include <SDL.h>
#include <stb_image.h>
#include <filesystem>
#include <fstream>
#include <vector>

namespace TextureLoader {
  inline bool LoadTextureFromMemory (const void* data, size_t data_size, SDL_Renderer* renderer,
                              SDL_Texture** out_texture, int* out_width, int* out_height) {
    int image_width = 0, image_height = 0, channels = 0;
    unsigned char* image_data = stbi_load_from_memory (static_cast<const unsigned char*> (data),
                                                       static_cast<int> (data_size), &image_width,
                                                       &image_height, &channels, 4);
    if (!image_data) {
      LOG_E_FMT ("Failed to load image from memory: %s", stbi_failure_reason ());
      return false;
    }

    SDL_Surface* surface
        = SDL_CreateRGBSurfaceFrom (image_data, image_width, image_height, 32, 4 * image_width,
                                    0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
    if (!surface) {
      LOG_E_FMT ("Failed to create SDL surface: %s", SDL_GetError ());
      stbi_image_free (image_data);
      return false;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface (renderer, surface);
    if (!texture) {
      LOG_E_FMT ("Failed to create SDL texture: %s", SDL_GetError ());
      SDL_FreeSurface (surface);
      stbi_image_free (image_data);
      return false;
    }

    *out_texture = texture;
    *out_width = image_width;
    *out_height = image_height;

    SDL_FreeSurface (surface);
    stbi_image_free (image_data);
    return true;
  }

  inline bool LoadTextureFromFile (const std::filesystem::path& file_path, SDL_Renderer* renderer,
                            SDL_Texture** out_texture, int* out_width, int* out_height) {
    std::ifstream file (file_path, std::ios::binary | std::ios::ate);
    if (!file) {
      LOG_E_FMT ("Failed to open file: %s", file_path.string ().c_str ());
      return false;
    }
    std::streamsize file_size = file.tellg ();
    if (file_size <= 0) {
      LOG_E_FMT ("File is empty or error reading file: %s", file_path.string ().c_str ());
      return false;
    }
    file.seekg (0, std::ios::beg);

    std::vector<unsigned char> buffer (static_cast<size_t> (file_size));
    if (!file.read (reinterpret_cast<char*> (buffer.data ()), file_size)) {
      LOG_E_FMT ("Failed to read file: %s", file_path.string ().c_str ());
      return false;
    }

    return LoadTextureFromMemory (buffer.data (), buffer.size (), renderer, out_texture, out_width,
                                  out_height);
  }
} // namespace TextureLoader
#endif // __TEXTURETOOLS_H__