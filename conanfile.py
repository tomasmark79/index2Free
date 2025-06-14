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
    name = "libcore"
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

        # libs/emscripten/emscripten_mainloop_stub.h content copied by hand to /src/bindings
        copy(self, "*opengl3*", os.path.join(self.dependencies["imgui"].package_folder,
            "res", "bindings"), os.path.join(self.source_folder, "src/bindings"))
        copy(self, "*sdl2*", os.path.join(self.dependencies["imgui"].package_folder,
             "res", "bindings"), os.path.join(self.source_folder, "src/bindings"))
        
        tc.generate()

    # Consuming recipe
    def configure(self):
        self.options["*"].shared = False # this replaced shared flag from SolutionController.py and works

    def requirements(self):
        self.requires("fmt/[~11.1]") # required by cpm package
        self.requires("imgui/1.91.8")
        self.requires("glm/1.0.1")
        
        if self.settings.os != "Emscripten":
            self.requires("glew/2.2.0")
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

    # TO DO 
    # # ----------------------------------------------------------    
    # # Creating basic library recipe
    # # Not recomended due complexity of this project template
    # # ----------------------------------------------------------
