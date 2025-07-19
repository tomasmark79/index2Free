import os, json, uuid
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, CMakeDeps
from conan.tools.system import package_manager
from conan.tools.files import copy

# DO NOT use cmake_layout(self) HERE!
# ------------------------------------------------- --
    # DotNameCpp is using self layout               --
    # to define build ouput layout!                 --
    # ├── Build                                     --
    #     ├── Artefacts - tarballs of installation  --
    #     ├── Install - final installation          --
    #     ├── Library - library build               --
    #     └── Standalone - standalone build         --
# ------------------------------------------------- --

class DotNameCppRecipe(ConanFile):
    name = "corelib"
    version = "1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    def generate(self): 
        tc = CMakeToolchain(self)
        self.update_cmake_presets("CMakePresets.json")

        if self.settings.os == "Emscripten":
            print("Emscripten detected by Conan build system")
            tc.variables["PLATFORM"] = "Web"
            tc.variables["CMAKE_EXE_LINKER_FLAGS"] = "-s USE_GLFW=3"
        
        # # Pro statické linkování runtime knihoven na Linuxu
        # if self.settings.os == "Linux" and not str(self.settings.compiler).startswith("Visual"):
        #     tc.variables["CMAKE_CXX_STANDARD_LIBRARIES"] = "-static-libgcc -static-libstdc++"
        #     tc.variables["CMAKE_CXX_IMPLICIT_LINK_LIBRARIES"] = "gcc_s;gcc;c;gcc_s;gcc"
        #     print("Configured static C++ runtime linking")

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
        self.options["*"].shared = False # this replaced shared flag from SolutionController.py and works
        
        # # Pro statické linkování - upravit system libs aby neobsahovaly stdc++
        # if self.settings.os == "Linux":
        #     # Přepsat system libs pro libmodplug aby neobsahoval stdc++
        #     self.options["libmodplug"].shared = False
    def requirements(self):
        self.requires("fmt/[~11.1]")            # required by cpm package
        self.requires("nlohmann_json/[~3.12]")  # required by DotNameUtils::JsonUtils
        self.requires("imgui/1.92.0")
        self.requires("glm/1.0.1")
        
        if self.settings.os != "Emscripten":
            #self.requires("libglvnd/1.5.0", override=True)  # Use libglvnd for OpenGL support
            #self.requires("opengl/system")
            #self.requires("glew/2.2.0")
            self.requires("sdl/2.32.2", override=True)  # Use the latest stable version of SDL
            self.requires("sdl_image/2.8.2")
            self.requires("sdl_ttf/2.24.0")
            self.requires("sdl_mixer/2.8.0")
            self.requires("sdl_net/2.2.0")

    def build_requirements(self):
        self.tool_requires("cmake/[>3.14]")

    def imports(self):
        self.copy("license*", dst="licenses", folder=True, ignore_case=True)

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

    # def system_requirements(self):
        # dnf = package_manager.Dnf(self)
        # dnf.install("SDL2-devel")
        # apt = package_manager.Apt(self)
        # apt.install(["libsdl2-dev"])
        # yum = package_manager.Yum(self)
        # yum.install("SDL2-devel")
        # brew = package_manager.Brew(self)
        # brew.install("sdl2")

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

    # TO DO 
    # # ----------------------------------------------------------    
    # # Creating basic library recipe
    # # Not recomended due complexity of this project template
    # # ----------------------------------------------------------
