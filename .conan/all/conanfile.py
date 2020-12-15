import os

from conans import ConanFile, CMake, tools

class SociConan(ConanFile):
    name = "soci"
    homepage = "https://github.com/SOCI/soci"
    url = "https://github.com/conan-io/conan-center-index"
    description = "The C++ Database Access Library "
    topics = ("C++", "database-library", "oracle", "postgresql", "mysql", "odbc", "db2", "firebird", "sqlite3", "boost" )
    license = "Boost"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"

    options = {
        "fPIC":       [True, False],
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
        "fPIC":       True,
        "shared":     False,
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
        prefix  = "Dependencies for "
        message = " not configured in this conan package, some features will be disabled."

        if self.options.sqlite3:
            self.requires("sqlite3/3.33.0")
        # ToDo add the missing dependencies for the backends
        if self.options.db2:
            self.output.warn(prefix + "DB2" + message)
        if self.options.odbc:
            self.output.warn(prefix + "ODBC" + message)
        if self.options.oracle:
            self.output.warn(prefix + "ORACLE" + message)
        if self.options.firebird:
            self.output.warn(prefix + "FIREBIRD" + message)
        if self.options.mysql:
            self.output.warn(prefix + "MYSQL" + message)
        if self.options.postgresql:
            self.output.warn(prefix + "POSTGRESQL" + message)

    def source(self):
        tools.get(**self.conan_data["sources"][self.version])
        os.rename(self.name + "-" + self.version, self._source_subfolder)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake

        self._cmake = CMake(self)

        self._cmake.definitions["SOCI_SHARED"]     = self.options.shared
        self._cmake.definitions["SOCI_STATIC"]     = self.options.static
        self._cmake.definitions["SOCI_EMPTY"]      = self.options.empty
        self._cmake.definitions["SOCI_TESTS"]      = self.options.tests
        self._cmake.definitions["SOCI_CXX11"]      = self.options.cxx11
        self._cmake.definitions["SOCI_SQLITE3"]    = self.options.sqlite3
        self._cmake.definitions["SOCI_DB2"]        = self.options.db2
        self._cmake.definitions["SOCI_ODBC"]       = self.options.odbc
        self._cmake.definitions["SOCI_ORACLE"]     = self.options.oracle
        self._cmake.definitions["SOCI_FIREBIRD"]   = self.options.firebird
        self._cmake.definitions["SOCI_MYSQL"]      = self.options.mysql
        self._cmake.definitions["SOCI_POSTGRESQL"] = self.options.postgresql

        if self.options.cxx11:
            self._cmake.definitions["CMAKE_CXX_STANDARD"] = "11" # ToDo review this. The standard should not be set here

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
        self.copy("LICENSE_1_0.txt", dst="licenses", src=self._source_subfolder)

    def package_info(self):
        self.cpp_info.includedirs = ['include']
        self.cpp_info.libdirs = ['lib', 'lib64']
        self.cpp_info.builddirs = ['cmake']

        self.cpp_info.libs = ["soci_core"]
        if self.options.empty:
            self.cpp_info.libs.append("soci_empty")
        if self.options.sqlite3:
            self.cpp_info.libs.append("soci_sqlite3")
        if self.options.oracle:
            self.cpp_info.libs.append("soci_oracle")
        if self.options.mysql:
            self.cpp_info.libs.append("soci_mysql")
        if self.options.postgresql:
            self.cpp_info.libs.append("soci_postgresql")
