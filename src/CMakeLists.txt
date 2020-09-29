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

add_library(websocketsecure_server
  ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/uuid.cpp
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
  nlohmann_json::nlohmann_json
)

add_executable(server 
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

target_link_libraries(server PRIVATE 
  cxxopts::cxxopts
  fmt::fmt
  websocketsecure_server
)