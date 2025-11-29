from conan import ConanFile
from conan.tools.cmake import cmake_layout

class DigitalClockV3Conan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    
    def requirements(self):
        self.requires("cpr/1.12.0")
        self.requires("nlohmann_json/3.12.0")
        self.requires("sdl/3.2.20")
        
    def layout(self):
        cmake_layout(self)
