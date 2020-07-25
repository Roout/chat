cmake_minimum_required (VERSION 3.16)

set(This client)
set(CMAKE_CXX_STANDARD 17)

set ( sources_files 
  "Client.cpp"
  "../classes/src/DoubleBuffer.cpp"
  "../classes/src/Request.cpp"
  "main.cpp"
)
set ( header_files
  "Client.hpp"
  "../classes/include/DoubleBuffer.hpp"
  "../classes/include/RequestType.hpp"
  "../classes/include/Request.hpp"
  "../classes/include/Utility.hpp"
)

add_library(${This}_lib STATIC ${sources_files} ${header_files})

add_executable (${This} ${sources_files} ${header_files})

target_include_directories(${This} 
  PUBLIC "../classes/include"
)

target_include_directories(${This}_lib 
  PUBLIC "../classes/include"
)

add_definitions(-DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB)

find_package(Boost "1.72.0" REQUIRED)

message(STATUS "Client perform search")
message(STATUS "BOOST_ROOT ${BOOST_ROOT}")
message(STATUS "Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS}")

if(Boost_FOUND)
  target_include_directories(${This} 
    PUBLIC ${Boost_INCLUDE_DIRS} 
  )
  target_include_directories(${This}_lib 
    PUBLIC ${Boost_INCLUDE_DIRS} 
  )
endif()