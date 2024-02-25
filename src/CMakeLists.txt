add_library(soci_interface INTERFACE)
add_library(SOCI::soci ALIAS soci_interface)

add_subdirectory(core)
add_subdirectory(backends)
