cmake_minimum_required (VERSION 3.16)

set(This server)
set(CMAKE_CXX_STANDARD 17)

set(headers)
set(sources)

list(APPEND headers
  "Server.hpp"
  "Session.hpp"
  "Chatroom.hpp"
  "RoomService.hpp"
  "RequestHandlers.hpp"
)

list(APPEND sources 
  "Server.cpp"
  "Session.cpp"
  "Chatroom.cpp"
  "RoomService.cpp"
  "RequestHandlers.cpp"
  "main.cpp"
)

add_library(${This}_lib STATIC ${headers} ${sources})

add_executable(${This} ${headers} ${sources})

set(client_targets 
  ${This} 
  ${This}_lib
)

foreach(ctarget ${client_targets})
  target_include_directories(${ctarget} 
    PRIVATE ${RAPIDJSON_INCLUDE_DIR}
    PRIVATE ${common_INCLUDE_DIRS}
  )
  target_link_libraries(${ctarget}
    PRIVATE common
  )
endforeach()

# Expose public includes to other
# subprojects through cache variable.
set(${This}_INCLUDE_DIRS 
  ${PROJECT_SOURCE_DIR}/server
  ${common_INCLUDE_DIRS}
  CACHE INTERNAL "${This}: Include Directories" FORCE
)
