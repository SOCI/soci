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


    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}ConfigVersion.cmake 
        COMPATIBILITY SameMajorVersion
    )

    install(FILES cmake/configs/${ARG_NAME}Config.cmake ${CMAKE_CURRENT_BINARY_DIR}/${ARG_NAME}ConfigVersion.cmake 
        DESTINATION ${SOCI_INSTALL_CMAKEDIR}
    )

    install(TARGETS ${ARG_TARGETS}
        EXPORT ${ARG_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} 
            COMPONENT ${ARG_NAME}_runtime
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}             
            COMPONENT          ${ARG_NAME}_runtime           
            NAMELINK_COMPONENT ${ARG_NAME}_development   
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}           
          COMPONENT   ${ARG_NAME}_development 
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT ${ARG_NAME}
        DESTINATION ${SOCI_INSTALL_CMAKEDIR}  
        NAMESPACE   SOCI::                    
        COMPONENT   ${ARG_NAME}_development
    )
endfunction()
