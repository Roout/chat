cmake_minimum_required (VERSION 3.16)

set(This client)
set(CMAKE_CXX_STANDARD 17)

set(headers)
set(sources)

list(APPEND headers
  "Client.hpp"
  "GUI.hpp"
)

list(APPEND sources 
  "Client.cpp"
  "main.cpp"
)

find_package(Threads REQUIRED)

# debug message
message(STATUS "Client debug message: ")
if(${Threads_FOUND})
  message(STATUS "Threads_FOUND is true")
  message(STATUS "CMAKE_THREAD_LIBS_INIT is ${CMAKE_THREAD_LIBS_INIT}")
else()
  message(STATUS "Threads_FOUND is false")
endif()
message(STATUS "\tHeaders: ${headers}")
message(STATUS "\tSources: ${sources}")

add_library(${This}_lib STATIC ${sources} ${headers})

add_executable (${This} ${sources} ${headers})

set(client_targets 
  ${This} 
  ${This}_lib
)

add_definitions(-DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB)

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

# add_definitions(-DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB)

# Expose public includes to other
# subprojects through cache variable.
set(${This}_INCLUDE_DIRS 
  ${PROJECT_SOURCE_DIR}/client
  CACHE INTERNAL "${This}: Include Directories" FORCE
)