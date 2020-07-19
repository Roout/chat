cmake_minimum_required (VERSION 3.16)

set(This request-tests)

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
    PUBLIC "../client-server/include"
)

target_link_libraries(${This} PUBLIC
    gtest_main
    client-server_lib
)

add_test(
    NAME ${This}
    COMMAND ${This}
)