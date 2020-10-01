cmake_minimum_required (VERSION 3.16)

set(This client)
set(CMAKE_CXX_STANDARD 17)

set(RAPIDJSON_DIR "../")
set(COMMON_INTERFACE_DIR "../")

include(${COMMON_INTERFACE_DIR}/classes/CMakeLists.txt)

set(headers)
set(sources)

list(APPEND headers
  "Client.hpp"
  "GUI.hpp"
  ${COMMON_INTERFACE_HEADERS}
)

list(APPEND sources 
  "Client.cpp"
  "main.cpp"
  ${COMMON_INTERFACE_SOURCES}
)

# debug message
message(STATUS "Client debug message: ")
message(STATUS "\tHeaders: ${headers}")
message(STATUS "\tSources: ${sources}")

add_library(${This}_lib STATIC ${sources} ${headers})

add_executable (${This} ${sources} ${headers})

target_include_directories(${This} 
  PUBLIC ${COMMON_INTERFACE_DIR}
  PRIVATE ${RAPIDJSON_DIR}
)

target_include_directories(${This}_lib 
  PUBLIC ${COMMON_INTERFACE_DIR}
  PRIVATE ${RAPIDJSON_DIR}
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

# Expose public includes to other
# subprojects through cache variable.
set(${This}_INCLUDE_DIRS 
  ${PROJECT_SOURCE_DIR}/client
  CACHE INTERNAL "${This}: Include Directories" FORCE
)