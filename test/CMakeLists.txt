find_package(Catch2 QUIET)
if (NOT ${Catch2_FOUND})
  conan_cmake_install(BUILD missing)
  
  find_package(Catch2 REQUIRED)
endif()
find_package(nlohmann_json REQUIRED)

add_executable(websocketsecure_server_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests_main.cpp
)

target_link_libraries(websocketsecure_server_tests
PRIVATE
    websocketsecure_server
    Catch2::Catch2
    nlohmann_json::nlohmann_json
)
add_test(NAME websocketsecure_server_tests COMMAND websocketsecure_server_tests)

if(MSVC)
  configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/msvc.runsettings.in
    ${CMAKE_SOURCE_DIR}/msvc.runsettings
  )
  
  target_sources(websocketsecure_server_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_client_info.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_message_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_uuid.hpp
  )

endif(MSVC)