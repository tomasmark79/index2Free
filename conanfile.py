import os, json, glob
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, CMakeDeps
from conan.tools.system import package_manager
from conan.tools.files import copy
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import patch
from conan.tools.files import replace_in_file

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
    # This template is using custom layout          --
    # to define build output layout!                --
    # ├── Build                                     --
    #     ├── Artefacts - tarballs of installation  --
    #     ├── Install - final installation          --
    #     ├── Library - library build               --
    #     └── Standalone - standalone build         --
# ------------------------------------------------- --

class ProjectTemplateRecipe(ConanFile):
    name = "corelib"
    exports_sources = "patches/*"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    def generate(self): 
        tc = CMakeToolchain(self)
        tc.variables["CMAKE_BUILD_TYPE"] = str(self.settings.build_type)
        
        tc.generate()

        # Update preset names behind tc.generate()
        self.update_cmake_presets("CMakePresets.json")

        # libs/emscripten/emscripten_mainloop_stub.h content copied by hand to /src/bindings behind tc.generate()
        copy(self, "*opengl3*", os.path.join(self.dependencies["imgui"].package_folder,
            "res", "bindings"), os.path.join(self.source_folder, "src/bindings"))
        copy(self, "*sdl2*", os.path.join(self.dependencies["imgui"].package_folder,
             "res", "bindings"), os.path.join(self.source_folder, "src/bindings"))
        
        # Patchování Conan CMake souborů pro odstranění stdc++ z system libs
        self.patch_remove_stdcpp_from_system_libs()
        
    # Consuming recipe
    def configure(self):
        # Force static linking for all dependencies (recommended for templates)
        self.options["*"].shared = False
       
        if self.settings.os == "Windows" and self.settings.compiler == "gcc":
            self.options["freetype"].with_png = False
            self.options["freetype"].with_brotli = False
            self.options["freetype"].with_zlib = False
            self.options["freetype"].with_bzip2 = False

        # Handle fPIC option for static libraries on non-Windows systems    
        if self.settings.os != "Windows":
            if self.options.fPIC:
                self.options["*"].fPIC = True

    def requirements(self):
        self.requires("m4/1.4.20", override=True)  # Custom build with upstream fix
        self.requires("fmt/[~11.1]") 
        self.requires("nlohmann_json/[~3.12]")
        self.requires("imgui/1.92.0")
        self.requires("glm/1.0.1")

        if self.settings.os != "Emscripten":
            self.requires("sdl/2.32.2", override=True)  # Use the latest stable version of SDL
            self.requires("sdl_image/2.8.2")
            self.requires("sdl_ttf/2.24.0")
            self.requires("sdl_mixer/2.8.0")
            self.requires("sdl_net/2.2.0")

            if self.settings.os == "Windows" and self.settings.compiler == "gcc":
                self.requires("glew/2.2.0")

            # if self.settings.arch == "armv8":            
            #     self.requires("libunwind/1.7.0", override=True)  # 1.8.0 is __asm__ __volatile__ ( error
            #     self.requires("libffi/3.4.8", override=True)  # Foreign Function Interface library







    # Optional: Define system requirements for Linux distributions    
    def imports(self):
        self.copy("license*", dst="licenses", folder=True, ignore_case=True)

    # ###################################################################
    # Functions Utilities - no need to change
    # ###################################################################

    # Dynamic change of names of CMakePresets.json - avoid name conflicts
    def update_cmake_presets(self, preset_file):
        """
        Updates CMake preset names to avoid conflicts during parallel builds.
        Preset names will be replaced with a generated name for simplicity.
        """
        preset_file = "CMakePresets.json"
        if os.path.exists(preset_file):
            with open(preset_file, "r", encoding="utf-8") as f:
                data = json.load(f)
            # Generate a unique preset name based on build settings
            preset_name = f"{str(self.settings.build_type).lower()}-{str(self.settings.os).lower()}-{self.settings.arch}-{self.settings.compiler}-{self.settings.compiler.version}"
            name_mapping = {}
            for preset in data.get("configurePresets", []):
                old_name = preset["name"]
                # Assign generated name directly
                new_name = preset_name
                preset["name"] = new_name
                # Update displayName to show the new preset name
                name_mapping[old_name] = new_name  # Uložení pro reference
            for preset in data.get("buildPresets", []):
                if preset["configurePreset"] in name_mapping:
                    # Map build presets to new preset name
                    preset["name"] = preset_name
                    preset["configurePreset"] = preset_name
            for preset in data.get("testPresets", []):
                if preset["configurePreset"] in name_mapping:
                    # Map test presets to new preset name
                    preset["name"] = preset_name
                    preset["configurePreset"] = preset_name
            with open(preset_file, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=4)

   # Patch Conan CMake files to remove stdc++ from SYSTEM_LIBS voiding static linking issues
    # call it as self.patch_remove_stdcpp_from_system_libs() in generate() or build() method
    def patch_remove_stdcpp_from_system_libs(self):
        """Odstranění stdc++ z SYSTEM_LIBS v generovaných Conan CMake souborech"""
        import glob
        import re
        
        # Najdi všechny *-*-x86_64-data.cmake soubory
        pattern = os.path.join(self.generators_folder or ".", "*-*-x86_64-data.cmake")
        for cmake_file in glob.glob(pattern):
            try:
                with open(cmake_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                # Nahraď "m stdc++" za "m" ve všech SYSTEM_LIBS_* variantách (DEBUG, RELEASE, atd.)
                modified_content = re.sub(
                    r'(set\([^_]*_SYSTEM_LIBS_[A-Z]+\s+[^)]*?)stdc\+\+([^)]*\))',
                    r'\1\2',
                    content
                )
                
                # Také nahraď v obecných SYSTEM_LIBS bez suffixu
                modified_content = re.sub(
                    r'(set\([^_]*_SYSTEM_LIBS\s+[^)]*?)stdc\+\+([^)]*\))',
                    r'\1\2',
                    modified_content
                )
                
                if modified_content != content:
                    with open(cmake_file, 'w', encoding='utf-8') as f:
                        f.write(modified_content)
                    print(f"Patched {cmake_file} - removed stdc++ from SYSTEM_LIBS")
                    
            except Exception as e:
                print(f"Warning: Could not patch {cmake_file}: {e}")