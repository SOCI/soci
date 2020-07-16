include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(SOCI_INSTALL_CMAKEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/Soci CACHE STRING "Path to Soci cmake files")

function(install_component)
    cmake_parse_arguments(
        ARG
        ""
        "NAME"
        "TARGETS"
        ${ARGN}
    )

    message(${PROJECT_BINARY_DIR}/${ARG_NAME}ConfigVersion.cmake)

    write_basic_package_version_file(${PROJECT_BINARY_DIR}/${ARG_NAME}ConfigVersion.cmake 
        COMPATIBILITY SameMajorVersion
    )

    configure_file(${PROJECT_SOURCE_DIR}/cmake/configs/${ARG_NAME}Config.cmake.in ${ARG_NAME}Config.cmake @ONLY)

    install(FILES ${PROJECT_BINARY_DIR}/${ARG_NAME}Config.cmake ${PROJECT_BINARY_DIR}/${ARG_NAME}ConfigVersion.cmake 
        DESTINATION ${SOCI_INSTALL_CMAKEDIR}
    )


    install(TARGETS ${ARG_TARGETS}
        EXPORT ${ARG_NAME}Targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} 
            COMPONENT ${ARG_NAME}_runtime
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}             
            COMPONENT          ${ARG_NAME}_runtime           
            NAMELINK_COMPONENT ${ARG_NAME}_development   
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}           
          COMPONENT   ${ARG_NAME}_development 
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT ${ARG_NAME}Targets
        DESTINATION ${SOCI_INSTALL_CMAKEDIR}  
        NAMESPACE   SOCI::                    
        COMPONENT   ${ARG_NAME}_development
    )
endfunction()
