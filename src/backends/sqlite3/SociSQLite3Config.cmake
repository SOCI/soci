include(CMakeFindDependencyMacro)

find_dependency(SOCI::Core)
find_dependency(SQLite3)

include("${CMAKE_CURRENT_LIST_DIR}/SociSQLite3Targets.cmake")
