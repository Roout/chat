cmake_minimum_required (VERSION 3.16)

set(This client-server)
set(CMAKE_CXX_STANDARD 17)

set ( sources_files 
  "src/server.cpp"
  "src/client.cpp"
  "main.cpp"
)
set ( header_files
  "include/server.hpp"
  "include/client.hpp"
  "include/helper.hpp"
  "include/DoubleBuffer.hpp"
)
add_definitions(-D_WIN32_WINNT=0x0601)

add_executable (${This} ${sources_files} ${header_files})

# disable autolinking in boost
# avoid LNK1104 libboost_system i.e. on Windows: http://stackoverflow.com/a/28902261/122441
add_definitions(-DBOOST_ALL_NO_LIB )

set (Boost_USE_STATIC_RUNTIME OFF) # ('s' tag)
set (Boost_USE_STATIC_LIBS ON)
set (Boost_USE_MULTITHREADED ON)

find_package(Boost "1.72.0" REQUIRED)

message(STATUS "BOOST_ROOT ${BOOST_ROOT}")
message(STATUS "Boost_INCLUDE_DIR ${Boost_INCLUDE_DIR}")
message(STATUS "Boost_LIBRARY_DIR ${Boost_LIBRARY_DIR}")
message(STATUS "Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost_LIBRARIES ${Boost_LIBRARIES}")

if(Boost_FOUND)
  target_include_directories(${This} 
    PUBLIC ${Boost_INCLUDE_DIRS} 
    PUBLIC ${Boost_INCLUDE_DIR} 
    PUBLIC include
  )
  target_link_libraries(${This} PUBLIC ${Boost_LIBRARIES})
endif()