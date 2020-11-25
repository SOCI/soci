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
        cmake.definitions["CMAKE_CXX_STANDARD"] = "11"
        cmake.definitions["SOCI_CXX11"]      = "ON"
        cmake.definitions["SOCI_EMPTY"]      = "ON"
        cmake.definitions["SOCI_SQLITE3"]    = "ON"
        cmake.definitions["SOCI_TESTS"]      = "ON"
        cmake.definitions["SOCI_STATIC"]     = "ON"
        cmake.definitions["SOCI_DB2"]        = "OFF"
        cmake.definitions["SOCI_ODBC"]       = "OFF"
        cmake.definitions["SOCI_ORACLE"]     = "OFF"
        cmake.definitions["SOCI_FIREBIRD"]   = "OFF"
        cmake.definitions["SOCI_MYSQL"]      = "OFF"
        cmake.definitions["SOCI_POSTGRESQL"] = "OFF"
        # cmake.definitions["SOCI_DB2_TEST_CONNSTR:STRING"]        = "DATABASE=${SOCI_USER}\\;hostname=${SOCI_DB2_HOST}\\;UID=${SOCI_DB2_USER}\\;PWD=${SOCI_DB2_PASS}\\;ServiceName=50000\\;Protocol=TCPIP\\;"
        # cmake.definitions["SOCI_FIREBIRD_TEST_CONNSTR:STRING"]   = "service=LOCALHOST:/tmp/soci.fdb user=${SOCI_USER} password=${SOCI_PASS}"
        # cmake.definitions["SOCI_MYSQL_TEST_CONNSTR:STRING"]      = "host=localhost db=${SOCI_USER} user=${SOCI_USER} password=${SOCI_PASS}"
        # cmake.definitions["SOCI_POSTGRESQL_TEST_CONNSTR:STRING"] = "host=localhost port=5432 dbname=${SOCI_USER} user=${SOCI_USER} password=${SOCI_PASS}"

        cmake.configure(source_folder="soci")
        cmake.build()
        cmake.install()

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
