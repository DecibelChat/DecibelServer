find_package(Catch2 QUIET)
if (NOT ${Catch2_FOUND})
  conan_cmake_install(BUILD missing)
  
  find_package(Catch2 REQUIRED)
endif()

add_executable(websocketsecure_server_tests_client_info
    ${CMAKE_CURRENT_SOURCE_DIR}/client_info.cpp
)
target_link_libraries(websocketsecure_server_tests_client_info
PRIVATE
    websocketsecure_server
    Catch2::Catch2
)
add_test(NAME websocketsecure_server_tests_client_info COMMAND websocketsecure_server_tests_client_info)

if(MSVC)
  configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/msvc.runsettings.in
    ${CMAKE_SOURCE_DIR}/msvc.runsettings
  )
endif(MSVC)