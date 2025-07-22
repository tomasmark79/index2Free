from conan import ConanFile

class OpenGLSystemFakeConan(ConanFile):
    name = "opengl"
    version = "system"
    
    def package_info(self):
        # Provide minimal OpenGL linkage for cross-compilation
        # Assume that target system will have OpenGL available
        self.cpp_info.system_libs = ["GL", "GLU"]
        self.cpp_info.includedirs = []
        
    def system_requirements(self):
        # Override system_requirements to do nothing for cross-compilation
        self.output.info("Fake opengl/system - skipping system requirements for cross-compilation")
        pass
