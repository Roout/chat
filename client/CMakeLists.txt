cmake_minimum_required (VERSION 3.16)

set(This client)
set(CMAKE_CXX_STANDARD 17)

set ( sources_files 
  "src/client.cpp"
  "../classes/src/DoubleBuffer.cpp"
  "../classes/src/Request.cpp"
  "main.cpp"
)
set ( header_files
  "include/client.hpp"
  "../classes/include/DoubleBuffer.hpp"
  "../classes/include/RequestType.hpp"
  "../classes/include/Request.hpp"
  "../classes/include/Utility.hpp"
)
add_definitions(-D_WIN32_WINNT=0x0601)

add_library(${This}_lib STATIC ${sources_files} ${header_files})

add_executable (${This} ${sources_files} ${header_files})

target_include_directories(${This} 
  PUBLIC "../classes/include"
)

target_include_directories(${This}_lib 
  PUBLIC "../classes/include"
)


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
  target_include_directories(${This}_lib 
    PUBLIC ${Boost_INCLUDE_DIRS} 
    PUBLIC ${Boost_INCLUDE_DIR} 
    PUBLIC include
  )

  target_link_libraries(${This} PUBLIC ${Boost_LIBRARIES})

  target_link_libraries(${This}_lib PUBLIC ${Boost_LIBRARIES})

endif()