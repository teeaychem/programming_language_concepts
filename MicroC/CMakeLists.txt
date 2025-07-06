cmake_minimum_required(VERSION 4.0)
project(mCLLVM VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(llvm)
find_package(fmt CONFIG REQUIRED)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

option(ALSO_BUILD_TESTS "Build tests" OFF)

flex_target(
    SCANNER
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.ll
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/scanner.hpp
    COMPILE_FLAGS "-v"
)
bison_target(
    PARSER
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.yy
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cc
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hh
    COMPILE_FLAGS "-v"
)
add_flex_bison_dependency(SCANNER PARSER)

file(GLOB AST_H src/AST/*.hh)
file(GLOB AST_I src/AST/*.cc)

add_library(${PROJECT_NAME} STATIC)

target_sources(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${BISON_PARSER_OUTPUTS}
        ${FLEX_SCANNER_OUTPUTS}
        ${AST_H}
        ${AST_I}
        src/driver.cc
    PUBLIC
        FILE_SET HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}/include
            FILES include/mcLLVM.hh include/driver.hh
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)

add_executable(mc)
target_sources(mc PRIVATE bin/main.cc)
target_include_directories(
    mc
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR} "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(mc PRIVATE fmt::fmt ${PROJECT_NAME})
add_subdirectory(src)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(installation)
