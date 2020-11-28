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
    generators = "cmake"

    options = {
        "shared":     [True, False],
        "cxx11":      [True, False],
        "sqlite3":    [True, False],
        "empty":      [True, False],
        "tests":      [True, False],
        "static":     [True, False],
        "db2":        [True, False],
        "odbc":       [True, False],
        "oracle":     [True, False],
        "firebird":   [True, False],
        "mysql":      [True, False],
        "postgresql": [True, False]
        }
    default_options = {
        "shared":     True,
        "cxx11":      False,
        "sqlite3":    False,
        "empty":      False,
        "tests":      False,
        "static":     False,
        "db2":        False,
        "odbc":       False,
        "oracle":     False,
        "firebird":   False,
        "mysql":      False,
        "postgresql": False
        }

    def requirements(self):
        if self.options.sqlite3:
            self.requires("sqlite3/3.33.0")
        # and so on for the rest of backends
        # ToDo add the dependencies for the missing backends

    def source(self):
        self.run("git clone https://github.com/spjuanjoc/soci.git -b feature/create-conan-package --single-branch")

    def build(self):
        cmake = CMake(self)
        cmake.definitions["SOCI_SHARED"]     = self.options.shared
        cmake.definitions["SOCI_CXX11"]      = self.options.cxx11
        cmake.definitions["SOCI_SQLITE3"]    = self.options.sqlite3
        cmake.definitions["SOCI_EMPTY"]      = self.options.empty
        cmake.definitions["SOCI_TESTS"]      = self.options.tests
        cmake.definitions["SOCI_STATIC"]     = self.options.static
        cmake.definitions["SOCI_DB2"]        = self.options.db2
        cmake.definitions["SOCI_ODBC"]       = self.options.odbc
        cmake.definitions["SOCI_ORACLE"]     = self.options.oracle
        cmake.definitions["SOCI_FIREBIRD"]   = self.options.firebird
        cmake.definitions["SOCI_MYSQL"]      = self.options.mysql
        cmake.definitions["SOCI_POSTGRESQL"] = self.options.postgresql

        if self.options.cxx11:
            cmake.definitions["CMAKE_CXX_STANDARD"] = "11"

        cmake.configure(source_folder="soci")
        cmake.build()
        cmake.install()

    def package(self):
        self.copy("*.h", dst="include", src="soci/include")
        self.copy("*soci*.lib", dst="lib", src="lib", keep_path=False, symlinks=True)
        self.copy("*soci*.so*", dst="lib", src="lib", keep_path=False, symlinks=True)
        self.copy("*.dylib", dst="lib", keep_path=False, symlinks=True)
        self.copy("*.a", dst="lib", src="lib", keep_path=False, symlinks=True)
        self.copy("*.dll", dst="bin", keep_path=False, symlinks=True)

    def package_info(self):
        self.cpp_info.libs = ["soci_core"]
        if self.options.empty:
            self.cpp_info.libs.append("soci_empty")
        if self.options.sqlite3:
            self.cpp_info.libs.append("soci_sqlite3")
        # And so on for the rest of libs
        # ToDo add the libs generated for the other backends

        self.cpp_info.includedirs = ['include']
        self.cpp_info.libdirs = ['lib']
