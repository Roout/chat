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
set ( Boost_DEBUG on)
set ( Boost_COMPILER "-vc1415")
set ( BOOST_ROOT "E:/Libs/boost/boost_1_67_0" )
#set ( BOOST_INCLUDEDIR "${BOOST_ROOT}" )
#set ( BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib" )
set (Boost_NO_SYSTEM_PATHS ON)
set (Boost_USE_MULTITHREADED ON)
set (Boost_USE_STATIC_LIBS OFF)
set (Boost_USE_STATIC_RUNTIME OFF)
set (BOOST_ALL_DYN_LINK ON)

find_package(Boost REQUIRED COMPONENTS chrono)

# Add source to this project's executable.
add_executable (client-server ${sources_files} ${header_files})

target_include_directories(client-server PUBLIC include)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(client-server ${Boost_LIBRARIES})
endif()
# TODO: Add tests and install targets if needed.
install(TARGETS client-server DESTINATION bin)