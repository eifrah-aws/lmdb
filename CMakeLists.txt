cmake_minimum_required(VERSION 3.10)
project(lmdb)

option(BUILD_EXAMPLES "Build examples" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_subdirectory(libraries/liblmdb)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
unset(BUILD_EXAMPLES CACHE)
