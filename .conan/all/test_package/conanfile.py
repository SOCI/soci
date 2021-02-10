import os
from conans import ConanFile, CMake, tools

class TestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    requires = ["catch2/2.13.3", "fmt/7.1.3", "sqlite3/3.33.0"]

    def configure(self):
        self.options["soci"].shared         = True
        self.options["soci"].empty          = True
        self.options["soci"].with_sqlite3   = True

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if not tools.cross_building(self.settings):
            self.run("ctest .")
