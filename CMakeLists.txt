cmake_minimum_required(VERSION 3.10)

project(DecibelServer)

include(CTest)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/conan.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/utils.cmake)

add_subdirectory(third_party)
add_subdirectory(src)

if (BUILD_TESTING)
  add_subdirectory(test)
endif (BUILD_TESTING)