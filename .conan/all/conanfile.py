import os

from conans import ConanFile, CMake, tools

class SociConan(ConanFile):
    name = "soci"
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

    _cmake = None

    @property
    def _source_subfolder(self):
        return "source_subfolder"

    @property
    def _build_subfolder(self):
        return "build_subfolder"

    def requirements(self):
        if self.options.sqlite3:
            self.requires("sqlite3/3.33.0")
        # and so on for the rest of backends
        # ToDo add the dependencies for the missing backends

    def source(self):
        tools.get(**self.conan_data["sources"][self.version])
        os.rename(self.name + "-" + self.version, self._source_subfolder)

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake

        self._cmake = CMake(self)

        self._cmake.definitions["SOCI_SHARED"]     = self.options.shared
        self._cmake.definitions["SOCI_CXX11"]      = self.options.cxx11
        self._cmake.definitions["SOCI_SQLITE3"]    = self.options.sqlite3
        self._cmake.definitions["SOCI_EMPTY"]      = self.options.empty
        self._cmake.definitions["SOCI_TESTS"]      = self.options.tests
        self._cmake.definitions["SOCI_STATIC"]     = self.options.static
        self._cmake.definitions["SOCI_DB2"]        = self.options.db2
        self._cmake.definitions["SOCI_ODBC"]       = self.options.odbc
        self._cmake.definitions["SOCI_ORACLE"]     = self.options.oracle
        self._cmake.definitions["SOCI_FIREBIRD"]   = self.options.firebird
        self._cmake.definitions["SOCI_MYSQL"]      = self.options.mysql
        self._cmake.definitions["SOCI_POSTGRESQL"] = self.options.postgresql

        if self.options.cxx11:
            self._cmake.definitions["CMAKE_CXX_STANDARD"] = "11"

        self._cmake.configure(
            source_folder=self._source_subfolder,
            build_folder=self._build_subfolder
        )

        return self._cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
        cmake.install()

    def package(self):
        include_folder  = os.path.join(self._source_subfolder, "include")
        lib_folder      = os.path.join(self._build_subfolder, "lib")
        bin_folder      = os.path.join(self._build_subfolder, "bin")

        self.copy("*.h",    dst="include", src=include_folder)
        self.copy("*soci*.lib", dst="lib", src=lib_folder, keep_path=False, symlinks=True)
        self.copy("*soci*.so*", dst="lib", src=lib_folder, keep_path=False, symlinks=True)
        self.copy("*.a",        dst="lib", src=lib_folder, keep_path=False, symlinks=True)
        self.copy("*.dylib",    dst="lib", src=lib_folder, keep_path=False, symlinks=True)
        self.copy("*.dll",      dst="bin", src=bin_folder, keep_path=False, symlinks=True)

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
