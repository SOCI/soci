soci_backend(Empty
  HEADERS soci-empty.h
  DESCRIPTION "SOCI backend skeleton for development of new backends"
  AUTHORS "Maciej Sobczak, Stephen Hutton"
  MAINTAINERS "Maciej Sobczak")

# TODO: check the flag inside soci_backend_test macro (to be implemented) --mloskot
if (SOCI_TESTS)
  add_subdirectory(test)
endif()
