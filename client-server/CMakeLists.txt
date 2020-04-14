cmake_minimum_required (VERSION 3.8)

set(This client-server)
set(CMAKE_CXX_STANDARD 17)

# Add source to this project's executable.

set ( sources_files 
  "src/client-server.cpp"
  "main.cpp"
)
set ( header_files
  "include/client-server.hpp"
)

# Add source to this project's executable.
add_executable (${This} ${sources_files} ${header_files})

target_include_directories(${This} PUBLIC include)