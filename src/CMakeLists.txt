# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${-fstandalone-debug}")


find_package(cxxopts QUIET)
if (NOT ${cxxopts_FOUND})
  conan_cmake_install(BUILD missing)
  
  find_package(cxxopts REQUIRED)
endif()

find_package(fmt QUIET)
if (NOT ${fmt_FOUND})
  conan_cmake_install(BUILD missing)
  
  find_package(fmt REQUIRED)
endif()

find_package(nlohmann_json QUIET)
if (NOT ${nlohmann_json_FOUND})
  conan_cmake_install(BUILD missing)
  
  find_package(nlohmann_json REQUIRED)
endif()

find_package(websocketpp QUIET)
if (NOT ${websocketpp_FOUND})
  conan_cmake_install(BUILD missing)
  
  find_package(websocketpp REQUIRED)
endif()

find_package(magic_enum QUIET)
if (NOT ${magic_enum_FOUND})
  conan_cmake_install(BUILD missing)
  
  find_package(magic_enum REQUIRED)
endif()

list(APPEND wss_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/ClientInfo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/MessageType.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/uuid.cpp
)

list(APPEND wss_headers
  ${CMAKE_CURRENT_SOURCE_DIR}/ClientInfo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/MessageType.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server.hpp
  ${CMAKE_CURRENT_SOURCE_DIR}/uuid.hpp
)

if (MSVC)
  list(APPEND wss_sources ${wss_headers})
endif (MSVC)

add_library(websocketsecure_server
  ${wss_sources}
)

target_compile_features(websocketsecure_server
  PUBLIC
    cxx_std_20
)

target_link_libraries(websocketsecure_server 
PUBLIC
  websocketpp::websocketpp
PRIVATE
  fmt::fmt
  magic_enum::magic_enum
  nlohmann_json::nlohmann_json
)

target_compile_options(websocketsecure_server 
PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:/bigobj>>
)

# TODO: Split public includes out in to include/websocketsecure_server folder
target_include_directories(websocketsecure_server
PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(server 
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(server PRIVATE 
  cxxopts::cxxopts
  fmt::fmt
  websocketsecure_server
)
