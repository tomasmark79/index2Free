#ifndef __GUISTRINGS_H__
#define __GUISTRINGS_H__

#include <string>
#include <string_view>

// Just one hardcoded string for the entire application
// Anothers are going to be loaded from a json file

inline constexpr std::string_view kAboutWebsite
    = R"(no good method to hardcode strings)";

// Function to convert a string_view to a string
inline std::string to_string (std::string_view sv) {
  return std::string (sv);
}

#endif // __GUISTRINGS_H__