include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(
        STATUS
        "CMAKE_INSTALL_PREFIX was set to default value: ${CMAKE_INSTALL_PREFIX}"
    )

    set(CMAKE_INSTALL_PREFIX
        "${CMAKE_SOURCE_DIR}/install"
        CACHE PATH
        "The library install location"
        FORCE
    )
endif()

message(STATUS "CMAKE_INSTALL_PREFIX is set to: ${CMAKE_INSTALL_PREFIX}")

install(
    TARGETS ${PROJECT_NAME}
    EXPORT "${PROJECT_NAME}Targets"
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILE_SET HEADERS
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION "${version}"
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

export(
    EXPORT ${PROJECT_NAME}Targets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PROJECT_NAME}Targets.cmake"
    NAMESPACE ${PROJECT_NAME}::
)
