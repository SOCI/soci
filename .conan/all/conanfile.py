import os
from conans import ConanFile, CMake, tools
from conans.tools import Version
from conans.errors import ConanInvalidConfiguration, ConanException

class SociConan(ConanFile):
    name = "soci"
    homepage = "https://github.com/SOCI/soci"
    url = "https://github.com/conan-io/conan-center-index"
    description = "The C++ Database Access Library "
    topics = ("cpp", "database-library", "oracle", "postgresql", "mysql", "odbc", "db2", "firebird", "sqlite3", "boost" )
    license = "BSL-1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    exports_sources = ["CMakeLists.txt"]
    _cmake = None

    options = {
        "fPIC":             [True, False],
        "shared":           [True, False],
        "empty":            [True, False],
        "with_sqlite3":     [True, False],
        "with_db2":         [True, False],
        "with_odbc":        [True, False],
        "with_oracle":      [True, False],
        "with_firebird":    [True, False],
        "with_mysql":       [True, False],
        "with_postgresql":  [True, False],
        "with_boost":       [True, False]
    }

    default_options = {
        "fPIC":             True,
        "shared":           False,
        "empty":            False,
        "with_sqlite3":     False,
        "with_db2":         False,
        "with_odbc":        False,
        "with_oracle":      False,
        "with_firebird":    False,
        "with_mysql":       False,
        "with_postgresql":  False,
        "with_boost":       False
    }

    @property
    def _source_subfolder(self):
        return "source_subfolder"

    @property
    def _minimum_cpp_standard(self):
        return 11

    @property
    def _minimum_compilers_version(self):
        return {
            "Visual Studio": "14",
            "gcc": "4.8",
            "clang": "3.8",
            "apple-clang": "8.0"
        }

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            del self.options.fPIC

        compiler = str(self.settings.compiler)
        compiler_version = Version(self.settings.compiler.version.value)
        tools.check_min_cppstd(self, self._minimum_cpp_standard)

        if compiler not in self._minimum_compilers_version:
            self.output.warn("{} recipe lacks information about the {} compiler support.".format(self.name, self.settings.compiler))
        elif compiler_version < self._minimum_compilers_version[compiler]:
            raise ConanInvalidConfiguration("{} requires a {} version >= {}".format(self.name, compiler, compiler_version))

    def requirements(self):
        prefix  = "Dependencies for"
        message = "not configured in this conan package."

        if self.options.with_sqlite3:
            self.requires("sqlite3/3.33.0")
        if self.options.with_db2:
            # self.requires("db2/0.0.0") # TODO add support for db2
            raise ConanInvalidConfiguration("{} DB2 {} ".format(prefix, message))
        if self.options.with_odbc:
            self.requires("odbc/2.3.7")
        if self.options.with_oracle:
            # self.requires("oracle_db/0.0.0") # TODO add support for oracle
            raise ConanInvalidConfiguration("{} ORACLE {} ".format(prefix, message))
        if self.options.with_firebird:
            # self.requires("firebird/0.0.0") # TODO add support for firebird
            raise ConanInvalidConfiguration("{} firebird {} ".format(prefix, message))
        if self.options.with_mysql:
            self.requires("libmysqlclient/8.0.17")
        if self.options.with_postgresql:
            self.requires("libpq/11.5")
        if self.options.with_boost:
            self.requires("boost/1.73.0")

    def source(self):
        tools.get(**self.conan_data["sources"][self.version])
        os.rename(self.name + "-" + self.version, self._source_subfolder)

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake

        self._cmake = CMake(self)

        self._cmake.definitions["SOCI_SHARED"]  = self.options.shared
        self._cmake.definitions["SOCI_TESTS"]   = False
        self._cmake.definitions["SOCI_CXX11"]   = True

        if self.options.shared:
            self._cmake.definitions["SOCI_STATIC"] = False

        self._cmake.definitions["SOCI_EMPTY"]       = self.options.empty
        self._cmake.definitions["WITH_SQLITE3"]     = self.options.with_sqlite3
        self._cmake.definitions["WITH_DB2"]         = self.options.with_db2
        self._cmake.definitions["WITH_ODBC"]        = self.options.with_odbc
        self._cmake.definitions["WITH_ORACLE"]      = self.options.with_oracle
        self._cmake.definitions["WITH_FIREBIRD"]    = self.options.with_firebird
        self._cmake.definitions["WITH_MYSQL"]       = self.options.with_mysql
        self._cmake.definitions["WITH_POSTGRESQL"]  = self.options.with_postgresql
        self._cmake.definitions["WITH_BOOST"]       = self.options.with_boost

        self._cmake.configure()

        return self._cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy("LICENSE_1_0.txt", dst="licenses", src=self._source_subfolder)

        cmake = self._configure_cmake()
        cmake.install()
        tools.rmdir(os.path.join(self.package_folder, "cmake"))

        if os.path.isdir(os.path.join(self.package_folder, "lib64")):
            if os.path.isdir(os.path.join(self.package_folder, "lib")):
                self.copy("*", dst="lib", src="lib64", keep_path=False, symlinks=True)
                tools.rmdir(os.path.join(self.package_folder, "lib64"))
            else:
                tools.rename(os.path.join(self.package_folder, "lib64"), os.path.join(self.package_folder, "lib"))

        os.remove(os.path.join(self.package_folder, "include", "soci", "soci-config.h.in"))

    def package_info(self):
        self.cpp_info.libs = ["soci_core"]
        if self.options.empty:
            self.cpp_info.libs.append("soci_empty")
        if self.options.with_sqlite3:
            self.cpp_info.libs.append("soci_sqlite3")
        if self.options.with_oracle:
            self.cpp_info.libs.append("soci_oracle")
        if self.options.with_mysql:
            self.cpp_info.libs.append("soci_mysql")
        if self.options.with_postgresql:
            self.cpp_info.libs.append("soci_postgresql")

        if self.settings.os == "Windows":
            for index, name in enumerate(self.cpp_info.libs):
                self.cpp_info.libs[index] = self._rename_library_win(name)

    def _rename_library_win(self, name):
        if self.options.shared:
            prefix = ""
        else:
            prefix = "lib"

        abi_version = tools.Version(self.version)
        sufix = "_{}_{}".format(abi_version.major, abi_version.minor)
        return "{}{}{}".format(prefix, name, sufix)
