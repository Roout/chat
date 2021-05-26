cmake_minimum_required (VERSION 3.12)

set(This tests)

set(CMAKE_CXX_STANDARD 17)

set(CLIENT_DIR ${client_INCLUDE_DIRS})
# NOTE: Boost's includes are transitively added through server_INCLUDE_DIRS.
set(SERVER_DIR ${server_INCLUDE_DIRS})

message(STATUS "Tests debug message:")
message(STATUS "\tSERVER_DIR: ${SERVER_DIR}")
message(STATUS "\tCLIENT_DIR: ${CLIENT_DIR}")

set(headers)
set(sources)

list(APPEND headers
  "message-tests.hpp"
  "single-client-messaging-tests.hpp"
)

list(APPEND sources 
  "all-tests.cpp"
)

find_package(OpenSSL REQUIRED)

add_executable(${This} ${sources} ${headers})

target_include_directories(${This} 
  PUBLIC ${RAPIDJSON_INCLUDE_DIR}
  PUBLIC ${SERVER_DIR}
  PUBLIC ${CLIENT_DIR}
  PUBLIC ${OPENSSL_INCLUDE_DIR}
)

add_definitions(
  -DBOOST_DATE_TIME_NO_LIB 
  -DBOOST_REGEX_NO_LIB 
  -D_WIN32_WINNT=0x0601
  -D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING
  -DRAPIDJSON_NOMEMBERITERATORCLASS
)

target_link_libraries(${This} PUBLIC
  gtest_main
  server_lib
  client_lib
  OpenSSL::SSL
  OpenSSL::Crypto
)

set(all_configs
  "dh2048.pem"
  "server.cfg"
  "server.crt"
  "server.key"
)

foreach(config ${all_configs})
  message(STATUS "\t\tTrying to copy to */settings/${config}")
  # Copy files for ctests
  configure_file("../settings/${config}" "settings/${config}" COPYONLY)
  # Copy files for Debug
  configure_file("../settings/${config}" "Debug/settings/${config}" COPYONLY)
  # Copy files for Release
  configure_file("../settings/${config}" "Release/settings/${config}" COPYONLY)
endforeach()

add_test(
  NAME ${This}
  COMMAND ${This}
)

target_compile_options(${This} PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:Clang>:-Wall>>
  $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:GNU>:-Wall>>
  $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:MSVC>:/W3>>
)