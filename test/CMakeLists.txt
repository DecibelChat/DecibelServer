find_conan_package(Catch2 CONANFILE ${CMAKE_CURRENT_SOURCE_DIR})
find_conan_package(nlohmann_json CONANFILE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(websocketsecure_server_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_server.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_server.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_client_info.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_message_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_server_messages_position.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_uuid.hpp
  )

endif(MSVC)

# doing some really ugly, misguided editing of source files because dependency's interface is not very friendly
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/uWebSockets/src/WebSocket.h file_data)
string(REPLACE "    void *getUserData()" "    virtual void *getUserData()" file_data "${file_data}")
string(REPLACE "    bool send(" "    virtual bool send(" file_data "${file_data}")
file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/uWebSockets/src/WebSocket.h "${file_data}")