add_executable(Test)
target_link_libraries(Test SOCI::soci_core SOCI::soci_postgresql)

target_sources(Test 
    PRIVATE 
        main.cpp 
)