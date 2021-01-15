# Need to manually incorporate uWebSockets in to cmake

set(uWebSockets_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/uWebSockets)
set(uSockets_ROOT ${uWebSockets_ROOT}/uSockets)

option(USE_OPENSSL             "Build networking backend with OpenSSL"                   ON)
option(USE_WOLFSSL             "Build networking backend with WolfSSL"                   OFF)
option(USE_LIBUV               "Build networking backend with libuv as event-loop"       ON)
option(USE_GCD                 "Build networking backend with libdispatch as event-loop" OFF)
option(USE_ZLIB                "Build networking backend with zlib compression enabled"  ON)
option(BUILD_WITH_PROXY        "Build networking backend with PROXY Protocol v2 support" OFF)
option(BUILD_VENDORED_EXAMPLES "Build examples for networking backend"                   OFF)
option(BUILD_VENDORED_TESTS    "Build tests for networking backend"                      OFF)

##################### uSockets #####################
list(APPEND uSockets_sources
  ${uSockets_ROOT}/src/bsd.c
  ${uSockets_ROOT}/src/context.c
  ${uSockets_ROOT}/src/loop.c
  ${uSockets_ROOT}/src/socket.c

  ${uSockets_ROOT}/src/crypto/openssl.c
  ${uSockets_ROOT}/src/crypto/sni_tree.cpp
  ${uSockets_ROOT}/src/crypto/wolfssl.c

  ${uSockets_ROOT}/src/eventing/epoll_kqueue.c
  ${uSockets_ROOT}/src/eventing/gcd.c
  ${uSockets_ROOT}/src/eventing/libuv.c
)

list(APPEND uSockets_headers
  ${uSockets_ROOT}/src/libusockets.h

  ${uSockets_ROOT}/src/internal/internal.h
  ${uSockets_ROOT}/src/internal/loop_data.h
  ${uSockets_ROOT}/src/internal/networking/bsd.h

  ${uSockets_ROOT}/src/internal/eventing/epoll_kqueue.h
  ${uSockets_ROOT}/src/internal/eventing/gcd.h
  ${uSockets_ROOT}/src/internal/eventing/libuv.h
)

list(APPEND uSockets_examples_sources
  ${uSockets_ROOT}/examples/echo_server.c
  ${uSockets_ROOT}/examples/hammer_test.c
  ${uSockets_ROOT}/examples/http_load_test.c
  ${uSockets_ROOT}/examples/http_server.c
  ${uSockets_ROOT}/examples/peer_verify_test.c
)

list(APPEND uSockets_tests_sources
  ${uSockets_ROOT}/tests/sni_test.c
)

if (MSVC)
 list(APPEND uSockets_sources ${uSockets_headers})
endif (MSVC)

add_library(uSockets
  ${uSockets_sources}
)

target_compile_features(uSockets
  PUBLIC
    cxx_std_17
    c_std_11
)

target_include_directories(uSockets
  PUBLIC
    ${uSockets_ROOT}/src
)

if (USE_OPENSLL AND USE_WOLFSSL)
    message(FATAL_ERROR "USE_OPENSSL and USE_WOLFSSL are mutally exclusive. Both cannot be enabled at the same time!")
elseif (USE_OPENSSL)
    find_conan_package(OpenSSL)
    target_link_libraries(uSockets
      PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
    )
    target_compile_definitions(uSockets
      PRIVATE
        LIBUS_USE_OPENSSL
    )
elseif (USE_WOLFSSL)
    message(FATAL_ERROR "WolfSSL backend not yet supported")
else()
    message(FATAL_ERROR "Neither USE_OPENSSL nor USE_WOLFSSL are true. One options MUST be true.")
endif()


if (USE_LIBUV AND USE_GCD)
    message(FATAL_ERROR "USE_LIBUV and USE_GCD are mutally exclusive. Both cannot be enabled at the same time!")
elseif (USE_LIBUV)
    find_conan_package(libuv)
    target_link_libraries(uSockets
      PRIVATE
        libuv::libuv
    )
    target_compile_definitions(uSockets
      PRIVATE
        LIBUS_USE_LIBUV
    )
elseif (USE_GCD)
    message(FATAL_ERROR "libdispatch backend not yet supported")
else()
    message(FATAL_ERROR "Neither USE_LIBUV nor USE_GCD are true. One options MUST be true.")
endif()

if (BUILD_VENDORED_EXAMPLES)
    foreach(example_file IN LISTS uSockets_examples_sources)
        get_filename_component(example_name ${example_file} NAME_WLE)
        set(example_fullname "uSockets_example_${example_name}")
        
        add_executable(${example_fullname} ${example_file})
        target_link_libraries(${example_fullname} PRIVATE uSockets)
    endforeach()
endif (BUILD_VENDORED_EXAMPLES)

if (BUILD_VENDORED_TESTS)
    foreach(test_file IN LISTS uSockets_tests_sources)
        get_filename_component(test_name ${test_file} NAME_WLE)
        set(test_fullname "uSockets_test_${test_name}")
        
        add_executable(${test_fullname} ${test_file})
        target_link_libraries(${test_fullname} PRIVATE uSockets)
        add_test(NAME ${test_fullname} COMMAND ${test_fullname})
    endforeach()
endif (BUILD_VENDORED_TESTS)

##################### uWebSockets #####################
list(APPEND uWebSockets_headers
  ${uWebSockets_ROOT}/src/App.h
  ${uWebSockets_ROOT}/src/AsyncSocket.h
  ${uWebSockets_ROOT}/src/AsyncSocketData.h
  ${uWebSockets_ROOT}/src/BloomFilter.h
  ${uWebSockets_ROOT}/src/HttpContext.h
  ${uWebSockets_ROOT}/src/HttpContextData.h
  ${uWebSockets_ROOT}/src/HttpParser.h
  ${uWebSockets_ROOT}/src/HttpResponse.h
  ${uWebSockets_ROOT}/src/HttpResponseData.h
  ${uWebSockets_ROOT}/src/HttpRouter.h
  ${uWebSockets_ROOT}/src/Loop.h
  ${uWebSockets_ROOT}/src/LoopData.h
  ${uWebSockets_ROOT}/src/MessageParser.h
  ${uWebSockets_ROOT}/src/Multipart.h
  ${uWebSockets_ROOT}/src/PerMessageDeflate.h
  ${uWebSockets_ROOT}/src/ProxyParser.h
  ${uWebSockets_ROOT}/src/QueryParser.h
  ${uWebSockets_ROOT}/src/TopicTree.h
  ${uWebSockets_ROOT}/src/Utilities.h
  ${uWebSockets_ROOT}/src/WebSocket.h
  ${uWebSockets_ROOT}/src/WebSocketContext.h
  ${uWebSockets_ROOT}/src/WebSocketContextData.h
  ${uWebSockets_ROOT}/src/WebSocketData.h
  ${uWebSockets_ROOT}/src/WebSocketExtensions.h
  ${uWebSockets_ROOT}/src/WebSocketHandshake.h
  ${uWebSockets_ROOT}/src/WebSocketProtocol.h

  ${uWebSockets_ROOT}/src/f2/function2.hpp
)

list(APPEND uWebSockets_examples_sources
  ${uWebSockets_ROOT}/examples/BroadcastingEchoServer.cpp
  ${uWebSockets_ROOT}/examples/EchoServer.cpp
  ${uWebSockets_ROOT}/examples/EchoServerThreaded.cpp
  ${uWebSockets_ROOT}/examples/HelloWorld.cpp
  ${uWebSockets_ROOT}/examples/HelloWorldThreaded.cpp
  ${uWebSockets_ROOT}/examples/HttpServer.cpp
  ${uWebSockets_ROOT}/examples/ServerName.cpp
  ${uWebSockets_ROOT}/examples/UpgradeAsync.cpp
  ${uWebSockets_ROOT}/examples/UpgradeSync.cpp
)

list(APPEND uWebSockets_tests_sources
  ${uWebSockets_ROOT}/tests/BloomFilter.cpp
  ${uWebSockets_ROOT}/tests/ExtensionsNegotiator.cpp
  ${uWebSockets_ROOT}/tests/HttpRouter.cpp
  ${uWebSockets_ROOT}/tests/TopicTree.cpp
)

add_library(uWebSockets INTERFACE)

if (MSVC)
  target_sources(uWebSockets INTERFACE ${uWebSockets_headers})
endif(MSVC)

if (BUILD_WITH_PROXY)
  target_compile_definitions(uWebSockets INTERFACE UWS_WITH_PROXY)
endif (BUILD_WITH_PROXY)

if (USE_ZLIB)
  find_conan_package(ZLIB)
  target_link_libraries(uWebSockets
    INTERFACE
      ZLIB::ZLIB
  )
else()
  target_compile_definitions(uWebSockets INTERFACE UWS_NO_ZLIB)
endif (USE_ZLIB)

target_compile_features(uWebSockets
  INTERFACE
    cxx_std_20
)

target_include_directories(uWebSockets
  INTERFACE
    ${uWebSockets_ROOT}/src
)

target_link_libraries(uWebSockets
  INTERFACE
    uSockets
)

if (BUILD_VENDORED_EXAMPLES)
    foreach(example_file IN LISTS uWebSockets_examples_sources)
        get_filename_component(example_name ${example_file} NAME_WLE)
        set(example_fullname "uWebSockets_example_${example_name}")
        
        add_executable(${example_fullname} ${example_file})
        target_link_libraries(${example_fullname} PRIVATE uWebSockets)
    endforeach()
endif (BUILD_VENDORED_EXAMPLES)

if (BUILD_VENDORED_TESTS)
    foreach(test_file IN LISTS uWebSockets_tests_sources)
        get_filename_component(test_name ${test_file} NAME_WLE)
        set(test_fullname "uWebSockets_test_${test_name}")
        
        add_executable(${test_fullname} ${test_file})
        target_link_libraries(${test_fullname} PRIVATE uWebSockets)
        add_test(NAME ${test_fullname} COMMAND ${test_fullname})
    endforeach()
endif (BUILD_VENDORED_TESTS)