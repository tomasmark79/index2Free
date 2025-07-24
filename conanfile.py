import os
import json
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps
from conan.tools.files import copy

# Template Configuration Notes:
# ----------------------------------------------------------
# 1. Change 'name' to match your project
# 2. Update requirements() with your actual dependencies
# 3. Uncomment system_requirements() if you need system packages
# 4. Consider adding validation for critical settings
# 5. This template avoids cmake_layout() for custom build structure
# ----------------------------------------------------------

# DO NOT use cmake_layout(self) HERE!
# ------------------------------------------------- --
#   This template is using custom layout          --
#   to define build output layout!                --
#   ├── Build                                     --
#       ├── Artefacts - tarballs of installation  --
#       ├── Install - final installation          --
#       ├── Library - library build               --
#       └── Standalone - standalone build         --
# ------------------------------------------------- --

class ProjectTemplateRecipe(ConanFile):
    name = "index2"
    exports_sources = "patches/*"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Imports
    def imports(self):
        self.copy("license*", dst="licenses", folder=True, ignore_case=True)

    # Generate CMake toolchain and presets
    def generate(self): 
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_BUILD_TYPE"] = str(self.settings.build_type)
        tc.generate()

        self.dotnameintegrated_update_cmake_presets()
        self.dotnameintegrated_patch_remove_stdcpp_from_system_libs()

        # Copy ImGui bindings after tc.generate()
        copy(self, "*opengl3*", 
             os.path.join(self.dependencies["imgui"].package_folder, "res", "bindings"), 
             os.path.join(self.source_folder, "src/bindings"))
        copy(self, "*sdl2*", 
             os.path.join(self.dependencies["imgui"].package_folder, "res", "bindings"), 
             os.path.join(self.source_folder, "src/bindings"))
        
    # Configure options and settings
    def configure(self):
        self.options["*"].shared = False
       
        # Handle fPIC option for static libraries on non-Windows systems    
        if self.settings.os != "Windows":
            if self.options.fPIC:
                self.options["*"].fPIC = True

        # ARM64/RPI4 specific optimizations
        if self.settings.arch == "armv8":
            self.options["sdl"].libunwind = False
            # Optimalizace pro ARM64
            self.options["sdl"].alsa = True
            self.options["sdl"].pulse = True

        if self.settings.os == "Windows" and self.settings.compiler == "gcc":
            self.options["freetype"].with_png = False
            self.options["freetype"].with_brotli = False
            self.options["freetype"].with_zlib = False
            self.options["freetype"].with_bzip2 = False

    # Requirements for dependencies
    def requirements(self):
        self.requires("m4/1.4.20@local/stable", override=True)  # Custom build with upstream fix  
        self.requires("fmt/11.0.2") 
        self.requires("nlohmann_json/3.11.3")
        self.requires("imgui/1.91.5")
        self.requires("glm/1.0.1")
        self.requires("libffi/3.4.8", override=True)  # Foreign Function Interface

        if self.settings.os != "Emscripten":
            self.requires("sdl/2.32.2", override=True)  # Latest stable SDL version
            self.requires("sdl_image/2.8.2")
            self.requires("sdl_ttf/2.24.0")
            self.requires("sdl_mixer/2.8.0")
            self.requires("sdl_net/2.2.0")

            if self.settings.os == "Windows" and self.settings.compiler == "gcc":
                self.requires("glew/2.2.0")

            # ARM-specific requirements (commented out)
            #if self.settings.arch == "armv8":
                 #self.requires("wayland/system", override=True)

    # ---------------------------------------------------------------------
    # Utility Functions - no need to change
    # ---------------------------------------------------------------------

    def dotnameintegrated_update_cmake_presets(self):
        """Dynamic change of names in CMakePresets.json to avoid name conflicts"""
        preset_file = "CMakePresets.json"
        if not os.path.exists(preset_file):
            return
            
        try:
            with open(preset_file, "r", encoding="utf-8") as f:
                data = json.load(f)
                
            preset_name = (f"{str(self.settings.build_type).lower()}-"
                          f"{str(self.settings.os).lower()}-"
                          f"{self.settings.arch}-"
                          f"{self.settings.compiler}-"
                          f"{self.settings.compiler.version}")
            
            # Collect old names from configurePresets for mapping
            name_mapping = {}
            for preset in data.get("configurePresets", []):
                old_name = preset["name"]
                preset["name"] = preset["displayName"] = preset_name
                name_mapping[old_name] = preset_name
                
            # Update buildPresets and testPresets in one pass
            for preset_type in ["buildPresets", "testPresets"]:
                for preset in data.get(preset_type, []):
                    if preset.get("configurePreset") in name_mapping:
                        preset["name"] = preset["configurePreset"] = preset_name
                        
            with open(preset_file, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=4)
                
        except (json.JSONDecodeError, IOError) as e:
            self.output.warn(f"Failed to update CMake presets: {e}")

    def dotnameintegrated_patch_remove_stdcpp_from_system_libs(self):
        """Remove stdc++ from SYSTEM_LIBS in generated Conan CMake files"""
        import glob
        import re
        
        # Find all *-data.cmake files for all platforms and architectures
        generators_path = self.generators_folder or "."
        patterns = [
            "*-data.cmake",          # General pattern for all files
            "*-*-*-data.cmake",      # Pattern for specific platforms (name-os-arch-data.cmake)
        ]
        
        cmake_files = []
        for pattern in patterns:
            cmake_files.extend(glob.glob(os.path.join(generators_path, pattern)))
            
        # Remove duplicates
        cmake_files = list(set(cmake_files))
        
        if not cmake_files:
            return
            
        # Compile regex pattern once for better performance
        system_libs_pattern = re.compile(
            r'(set\([^_]*_SYSTEM_LIBS(?:_[A-Z]+)?\s+[^)]*?)stdc\+\+([^)]*\))', 
            re.MULTILINE
        )
        
        patched_count = 0
        for cmake_file in cmake_files:
            try:
                with open(cmake_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                # Replace all occurrences of stdc++ in SYSTEM_LIBS
                modified_content = system_libs_pattern.sub(r'\1\2', content)
                
                if modified_content != content:
                    with open(cmake_file, 'w', encoding='utf-8') as f:
                        f.write(modified_content)
                    self.output.info(f"Patched {cmake_file} - removed stdc++ from SYSTEM_LIBS")
                    patched_count += 1
                    
            except (IOError, OSError) as e:
                self.output.warn(f"Could not patch {cmake_file}: {e}")
        
        if patched_count > 0:
            self.output.info(f"Successfully patched {patched_count} CMake files")