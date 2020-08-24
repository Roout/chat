cmake_minimum_required (VERSION 3.16)

set(This tests)

set(CMAKE_CXX_STANDARD 17)

set(sources_files 
  "all-tests.cpp"
)
set(header_files
  "message-tests.hpp"
  "single-client-messaging-tests.hpp"

  "../rapidjson/document.h"
  "../rapidjson/writer.h"
  "../rapidjson/stringbuffer.h"
)

enable_testing()

add_executable(${This} ${sources_files} ${header_files})

target_include_directories(${This} 
    PUBLIC "../classes"
    PUBLIC "../server"
    PUBLIC "../client"
    PUBLIC "../rapidjson"
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