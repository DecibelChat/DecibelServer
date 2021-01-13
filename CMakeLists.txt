cmake_minimum_required(VERSION 3.10)

project(DecibelServer)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan.cmake)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)