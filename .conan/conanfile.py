from conans import ConanFile, CMake, tools

class SociConan(ConanFile):
    name = "soci"
    version = "4.0.1"
    license = "Boost"
    author = "Maciej Sobczak maciej@msobczak"
    url = "https://github.com/SOCI/soci"
    description = "The C++ Database Access Library "
    topics = ("C++", "database-library", "oracle", "postgresql", "mysql", "odbc", "db2", "firebird", "sqlite3", "boost" )
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    generators = "cmake"

    def source(self):
        self.run("git clone https://github.com/spjuanjoc/soci.git -b feature/create-conan-package --single-branch")

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_folder="soci")
        cmake.build()

    def package(self):
        self.copy("*.h", dst="include", src="soci/include")
        self.copy("*soci.lib", dst="lib", keep_path=False)
        self.copy("*.dll", dst="bin", keep_path=False)
        self.copy("*.so", dst="lib", keep_path=False)
        self.copy("*.dylib", dst="lib", keep_path=False)
        self.copy("*.a", dst="lib", keep_path=False)

    def package_info(self):
        self.cpp_info.libs = ["soci_sqlite3"]
        self.cpp_info.libs = ["soci_core"]
        self.cpp_info.libs = ["soci_core"]
        self.cpp_info.libs = ["soci_empty"]
        self.cpp_info.libs = ["soci_sqlite3"]
