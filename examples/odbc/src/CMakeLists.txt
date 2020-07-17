add_executable(Test)
target_link_libraries(Test SOCI::soci_core SOCI::soci_odbc)

target_sources(Test 
    PRIVATE 
        main.cpp 
)