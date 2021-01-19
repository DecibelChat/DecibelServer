# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${-fstandalone-debug}")

option(INSECURE_SERVER "Build a server that does not use SSL. May be useful for local machine testing." OFF)

find_conan_package(cxxopts)
find_conan_package(fmt)
find_conan_package(magic_enum)
find_conan_package(nlohmann_json)
find_conan_package(spdlog)

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
  uWebSockets
PRIVATE
  fmt::fmt
  magic_enum::magic_enum
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)

target_compile_options(websocketsecure_server 
PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:/bigobj>>
)

if (INSECURE_SERVER)
  target_compile_definitions(websocketsecure_server
  PUBLIC
    WS_NO_TLS
  )
endif (INSECURE_SERVER)

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
