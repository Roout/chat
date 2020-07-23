cmake_minimum_required (VERSION 3.16)

set(This tests)

set(CMAKE_CXX_STANDARD 17)

set(sources_files 
  "all-tests.cpp"
)
set(header_files
  "request-tests.hpp"
)

enable_testing()

add_executable(${This} ${sources_files} ${header_files})

target_include_directories(${This} 
    PUBLIC "../classes/include"
    PUBLIC "../server/include"
    PUBLIC "../client/include"
)

target_link_libraries(${This} PUBLIC
    gtest_main
    server_lib
    client_lib
)

add_test(
    NAME ${This}
    COMMAND ${This}
)