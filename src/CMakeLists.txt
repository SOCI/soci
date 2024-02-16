add_library(soci_shared_interface INTERFACE)
add_library(SOCI::shared::soci ALIAS soci_shared_interface)

add_library(soci_static_interface INTERFACE)
add_library(SOCI::static::soci ALIAS soci_static_interface)

add_library(soci_interface INTERFACE)
add_library(SOCI::soci ALIAS soci_interface)

if (SOCI_STATIC AND SOCI_SHARED)
  if (BUILD_SHARED_LIBS)
    target_link_libraries(soci_interface INTERFACE SOCI::shared::soci)
  else()
    target_link_libraries(soci_interface INTERFACE SOCI::static::soci)
  endif()
elseif(SOCI_STATIC)
  target_link_libraries(soci_interface INTERFACE SOCI::static::soci)
else()
  target_link_libraries(soci_interface INTERFACE SOCI::shared::soci)
endif()


add_subdirectory(core)
add_subdirectory(backends)
