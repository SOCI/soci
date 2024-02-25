
add_library(soci_empty
  ${SOCI_LIB_TYPE}
    "blob.cpp"
    "factory.cpp"
    "row-id.cpp"
    "session.cpp"
    "standard-into-type.cpp"
    "standard-use-type.cpp"
    "statement.cpp"
    "vector-into-type.cpp"
    "vector-use-type.cpp"
)

target_link_libraries(soci_empty PUBLIC SOCI::Core)

target_include_directories(soci_empty
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

add_library(SOCI::Empty ALIAS soci_empty)

target_link_libraries(soci_interface INTERFACE SOCI::Empty)
