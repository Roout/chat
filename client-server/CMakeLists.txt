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
#set ( Boost_DEBUG on)

# Add source to this project's executable.
add_executable (client-server ${sources_files} ${header_files})

target_include_directories(client-server PUBLIC include)

# disable autolinking in boost
add_definitions(-DBOOST_ALL_NO_LIB )  # avoid LNK1104 libboost_system i.e. on Windows: http://stackoverflow.com/a/28902261/122441

#if Boost_USE_STATIC_LIBS is OFF then need to add *.dll to directory with *.exe file
set (Boost_USE_STATIC_LIBS ON) # works only with BOOST_ALL_NO_LIB
set (Boost_USE_MULTITHREADED ON)

set ( BOOST_ROOT "E:/Libs/boost/boost_1_68_0" )
set ( BOOST_INCLUDEDIR "${BOOST_ROOT}" )
set ( BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/Win64/lib" )

find_package(Boost REQUIRED COMPONENTS system chrono)

message(" ** Libraries: ${Boost_LIBRARIES}")

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(client-server ${Boost_LIBRARIES})
endif()
# TODO: Add tests and install targets if needed.
install(TARGETS client-server DESTINATION bin)