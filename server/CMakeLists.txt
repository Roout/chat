cmake_minimum_required (VERSION 3.16)

set(This server)
set(CMAKE_CXX_STANDARD 17)

set(headers)
set(sources)

list(APPEND headers
  "Server.hpp"
  "Session.hpp"
  "Connection.hpp"
  "Chatroom.hpp"
  "RoomService.hpp"
  "RequestHandlers.hpp"
)

list(APPEND sources 
  "Server.cpp"
  "Session.cpp"
  "Connection.cpp"
  "Chatroom.cpp"
  "RoomService.cpp"
  "RequestHandlers.cpp"
  "main.cpp"
)

add_library(${This}_lib STATIC ${headers} ${sources})

add_executable(${This} ${headers} ${sources})

find_package(Threads REQUIRED)

# debug message
message(STATUS "Server debug message: ")
if(${Threads_FOUND})
  message(STATUS "Threads_FOUND is true")
  message(STATUS "CMAKE_THREAD_LIBS_INIT is ${CMAKE_THREAD_LIBS_INIT}")
else()
  message(STATUS "Threads_FOUND is false")
endif()

set(client_targets 
  ${This} 
  ${This}_lib
)

add_definitions(-DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB -D_WIN32_WINNT=0x0601)

foreach(ctarget ${client_targets})
  target_include_directories(${ctarget} 
    PRIVATE ${RAPIDJSON_INCLUDE_DIR}
    PRIVATE ${common_INCLUDE_DIRS}
  )
  target_link_libraries(${ctarget}
    PRIVATE common
    PRIVATE ${CMAKE_THREAD_LIBS_INIT}
  )
endforeach()

# Expose public includes to other
# subprojects through cache variable.
set(${This}_INCLUDE_DIRS 
  ${PROJECT_SOURCE_DIR}/server
  ${common_INCLUDE_DIRS}
  CACHE INTERNAL "${This}: Include Directories" FORCE
)
