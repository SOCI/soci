add_library(Catch::Catch IMPORTED INTERFACE GLOBAL)
target_include_directories(Catch::Catch INTERFACE .)

# Request CATCH to disable all the C++11 features
if (NOT SOCI_WITH_CXX11)
  target_compile_definitions(Catch::Catch INTERFACE CATCH_CONFIG_NO_CPP11)
endif()