cmake_minimum_required(VERSION 4.0)
project(mCLLVM VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

option(ALSO_BUILD_TESTS "Build tests" OFF)

# flex and bison

flex_target(
    SCANNER
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scanner.ll
    ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/scanner.hpp
    # COMPILE_FLAGS "-v"
)
bison_target(
    PARSER
    ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.yy
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cc
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hh
    #  COMPILE_FLAGS "-v"
)
add_flex_bison_dependency(SCANNER PARSER)

# AST files

file(GLOB AST_H src/AST/*.hh)
file(GLOB AST_I src/AST/*.cc)

# Source files

file(GLOB SRC_I src/*.cc)

# LLVM

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(LLVM_LIBS
  Core
  ExecutionEngine
  Interpreter
  MC
  MCJIT
  Object
  OrcJIT
  Support
  TargetParser
  native
  nativecodegen
)

# library setup

add_library(${PROJECT_NAME} STATIC)

target_sources(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${BISON_PARSER_OUTPUTS}
        ${FLEX_SCANNER_OUTPUTS}
        ${AST_H}
        ${AST_I}
        ${SRC_I}
    PUBLIC
        FILE_SET HEADERS
            BASE_DIRS ${PROJECT_SOURCE_DIR}/include
            FILES include/mcLLVM.hh include/driver.hh include/CodegenLLVM.hh
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LLVM_LIBS})

add_subdirectory(src)

# mc

add_executable(mc)
target_sources(mc PRIVATE bin/main.cc)
target_include_directories(
    mc
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(mc PRIVATE ${PROJECT_NAME} ${LLVM_LIBS})

# collatz

add_executable(collatz)
target_sources(collatz PRIVATE bin/collatz.cc)
target_link_libraries(collatz PRIVATE ${PROJECT_NAME} ${LLVM_LIBS})

# other things

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(installation)

execute_process(
    COMMAND
        ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)
