import os, json, uuid
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, CMakeDeps
from conan.tools.system import package_manager
from conan.tools.files import copy
from conan.errors import ConanInvalidConfiguration

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
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    def generate(self): 
        tc = CMakeToolchain(self)
        self.update_cmake_presets("CMakePresets.json")

        # libs/emscripten/emscripten_mainloop_stub.h content copied by hand to /src/bindings
        copy(self, "*opengl3*", os.path.join(self.dependencies["imgui"].package_folder,
            "res", "bindings"), os.path.join(self.source_folder, "src/bindings"))
        copy(self, "*sdl2*", os.path.join(self.dependencies["imgui"].package_folder,
             "res", "bindings"), os.path.join(self.source_folder, "src/bindings"))
        
        tc.generate()

        # Patchování Conan CMake souborů pro odstranění stdc++ z system libs
        self.patch_remove_stdcpp_from_system_libs()
        
    # Consuming recipe
    def configure(self):
        # Force static linking for all dependencies (recommended for templates)
        self.options["*"].shared = False
        
        # if mingw used
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
        # Core dependencies - adjust as needed for your project
        self.requires("fmt/[~11.1]")            # Modern formatting library
        self.requires("nlohmann_json/[~3.12]")  # JSON parsing library
        self.requires("imgui/1.92.0")
        self.requires("glm/1.0.1")

        if self.settings.os != "Emscripten":
            # if mingw used
            if self.settings.os == "Windows" and self.settings.compiler == "gcc":
                self.requires("libpng/1.6.50", override=True)
                self.requires("libiconv/1.17", override=True)
                self.requires("glew/2.2.0")

                
            self.requires("sdl/2.32.2", override=True)  # Use the latest stable version of SDL
            self.requires("sdl_image/2.8.2")
            self.requires("sdl_ttf/2.24.0")
            self.requires("sdl_mixer/2.8.0")
            self.requires("sdl_net/2.2.0")

        # Additional dependencies - uncomment as needed:
        # self.requires("gtest/1.16.0")           # Google Test (if CPM not used)
        # self.requires("spdlog/[~1.12]")         # Logging library
        # self.requires("zlib/[~1.3]")            # Compression library
        # self.requires("yaml-cpp/0.8.0")         # YAML parsing
        # self.requires("boost/[~1.82]")          # Boost libraries

    #def build_requirements(self):
        # self.tool_requires("cmake/[>3.14]")

    # def system_requirements(self):
        # dnf = package_manager.Dnf(self)
        # dnf.install("SDL2-devel")
        # apt = package_manager.Apt(self)
        # apt.install(["libsdl2-dev"])

    def imports(self):
        self.copy("license*", dst="licenses", folder=True, ignore_case=True)







    # ###################################################################
    # Functions Utilities - no need to change
    # ###################################################################

    # Dynamic change of names of CMakePresets.json - avoid name conflicts
    def update_cmake_presets(self, preset_file):
        if os.path.exists(preset_file):
            with open(preset_file, "r", encoding="utf-8") as f:
                data = json.load(f)
            build_suffix = f"{self.settings.arch}-{uuid.uuid4().hex[:8]}"
            name_mapping = {}
            for preset in data.get("configurePresets", []):
                old_name = preset["name"]
                new_name = f"{old_name}-{build_suffix}"
                preset["name"] = new_name
                preset["displayName"] = f"{preset['displayName']} ({build_suffix})"
                name_mapping[old_name] = new_name  # Uložení pro reference
            for preset in data.get("buildPresets", []):
                if preset["configurePreset"] in name_mapping:
                    preset["name"] = name_mapping[preset["configurePreset"]]
                    preset["configurePreset"] = name_mapping[preset["configurePreset"]]
            for preset in data.get("testPresets", []):
                if preset["configurePreset"] in name_mapping:
                    preset["name"] = name_mapping[preset["configurePreset"]]
                    preset["configurePreset"] = name_mapping[preset["configurePreset"]]
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