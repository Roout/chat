# CMakeList.txt : CMake project for client-server, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.

set ( sources_files 
  "src/client-server.cpp"
  "main.cpp"
)
set ( header_files
  "include/client-server.h"
)

# Add source to this project's executable.
add_executable (client-server ${sources_files} ${header_files})

target_include_directories(client-server PUBLIC include)

# TODO: Add tests and install targets if needed.
install(TARGETS client-server DESTINATION bin)