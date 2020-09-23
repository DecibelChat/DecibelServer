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

add_executable(server 
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/server.cpp
)

target_compile_features(server 
  PRIVATE 
    cxx_std_20
)
# target_compile_options(server 
#   PRIVATE
#   $<$<CONFIG:Debug>:-fstandalone-debug>
# )
target_link_libraries(server PRIVATE 
  cxxopts::cxxopts
  fmt::fmt
  nlohmann_json::nlohmann_json
  websocketpp::websocketpp
)