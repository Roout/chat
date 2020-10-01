cmake_minimum_required (VERSION 3.16)

set(This server)
set(CMAKE_CXX_STANDARD 17)

set(RAPIDJSON_DIR "../")
set(COMMON_INTERFACE_DIR "../")

include(${COMMON_INTERFACE_DIR}/classes/CMakeLists.txt)

set(headers)
set(sources)

list(APPEND headers
  "Server.hpp"
  "Session.hpp"
  "Chatroom.hpp"
  "RoomService.hpp"
  "RequestHandlers.hpp"
  ${COMMON_INTERFACE_HEADERS}
)

list(APPEND sources 
  "Server.cpp"
  "Session.cpp"
  "Chatroom.cpp"
  "RoomService.cpp"
  "RequestHandlers.cpp"
  "main.cpp"
  ${COMMON_INTERFACE_SOURCES}
)

add_library (${This}_lib STATIC ${headers} ${sources})

add_executable (${This} ${headers} ${sources})

target_include_directories(${This}
  PUBLIC ${COMMON_INTERFACE_DIR}
  PUBLIC ${RAPIDJSON_DIR}
)
target_include_directories(${This}_lib
  PUBLIC ${COMMON_INTERFACE_DIR}
  PUBLIC ${RAPIDJSON_DIR}
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

# Expose public includes (including Boost transitively) to other
# subprojects through cache variable.
set(${This}_INCLUDE_DIRS 
  ${PROJECT_SOURCE_DIR}/server
  ${Boost_INCLUDE_DIRS}
  CACHE INTERNAL "${This}: Include Directories" FORCE
)