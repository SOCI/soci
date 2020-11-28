import os

from conans import ConanFile, CMake, tools

class SociTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    requires = "sqlite3/3.33.0"

    def configure(self):
        self.options["soci"].cxx11   = True
        self.options["soci"].sqlite3 = True
        self.options["soci"].empty   = True

    def requirements(self):
        self.requires("sqlite3/3.33.0")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def imports(self):
        self.copy("*.dll", dst="bin", src="bin")
        self.copy("*.dylib*", dst="bin", src="lib")
        self.copy('*.so*', dst='bin', src='lib')
        self.copy("*.a", dst="lib", src='lib')

    def test(self):
        if not tools.cross_building(self):
            self.run(".%sexample" % os.sep)
