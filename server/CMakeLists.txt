cmake_minimum_required (VERSION 3.16)

set(This server)
set(CMAKE_CXX_STANDARD 17)

set ( sources_files 
  "Server.cpp"
  "Session.cpp"
  "Chatroom.cpp"
  "RoomService.cpp"
  "RequestHandlers.cpp"
  "../classes/DoubleBuffer.cpp"
  "../classes/Message.cpp"
  "../classes/User.cpp"
  "main.cpp"
)
set ( header_files
  "../classes/User.hpp"
  "../classes/Log.hpp"
  "../classes/DoubleBuffer.hpp"
  "../classes/QueryType.hpp"
  "../classes/Message.hpp"
  "../classes/Utility.hpp"

  "Server.hpp"
  "Session.hpp"
  "Chatroom.hpp"
  "RoomService.hpp"
  "RequestHandlers.hpp"

  "../rapidjson/document.h"
  "../rapidjson/writer.h"
  "../rapidjson/stringbuffer.h"
)

add_library (${This}_lib STATIC ${sources_files} ${header_files})

add_executable (${This} ${sources_files} ${header_files})

target_include_directories(${This}
  PUBLIC "../classes"
  PUBLIC "../rapidjson"
)
target_include_directories(${This}_lib
  PUBLIC "../classes"
  PUBLIC "../rapidjson"
)

add_definitions(-DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB)

find_package(Boost "1.72.0" REQUIRED)

message(STATUS "Server perform search")
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
