cmake_minimum_required (VERSION 3.16)

set(This tests)

set(CMAKE_CXX_STANDARD 17)

set(CLIENT_DIR ${client_INCLUDE_DIRS})
# NOTE: Boost's includes are transitively added through server_INCLUDE_DIRS.
set(SERVER_DIR ${server_INCLUDE_DIRS})

message(STATUS "SERVER_DIR: ${SERVER_DIR}")
message(STATUS "CLIENT_DIR: ${CLIENT_DIR}")

set(headers)
set(sources)

list(APPEND headers
  "message-tests.hpp"
  "single-client-messaging-tests.hpp"
)

list(APPEND sources 
  "all-tests.cpp"
)

add_executable(${This} ${sources} ${headers})

target_include_directories(${This} 
  PUBLIC ${RAPIDJSON_INCLUDE_DIR}
  PUBLIC ${SERVER_DIR}
  PUBLIC ${CLIENT_DIR}
)

add_definitions(-DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB)

target_link_libraries(${This} PUBLIC
  gtest_main
  server_lib
  client_lib
)

add_test(
  NAME ${This}
  COMMAND ${This}
)