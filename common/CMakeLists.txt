cmake_minimum_required (VERSION 3.16)

set(This common)

project(${This} CXX)

set(CMAKE_CXX_STANDARD 17)

set(COMMON_INTERFACE_HEADERS
	include/DoubleBuffer.hpp
	include/Log.hpp
	include/Message.hpp
	include/QueryType.hpp
	include/User.hpp
	include/Utility.hpp
)

set(COMMON_INTERFACE_SOURCES
	sources/DoubleBuffer.cpp
	sources/Message.cpp
	sources/User.cpp
)

add_library(${This} STATIC 
	${COMMON_INTERFACE_HEADERS} 
	${COMMON_INTERFACE_SOURCES}
)

add_dependencies(${This} rapidjson)

target_include_directories(${This}
	PUBLIC "include"
	PUBLIC ${RAPIDJSON_INCLUDE_DIR}
)

add_definitions(-DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB)

find_package(Boost "1.72.0" REQUIRED)

message(STATUS "Common perform search")
message(STATUS "BOOST_ROOT ${BOOST_ROOT}")
message(STATUS "Boost_INCLUDE_DIRS ${Boost_INCLUDE_DIRS}")

if(Boost_FOUND)
	target_include_directories(${This} 
		PUBLIC ${Boost_INCLUDE_DIRS} 
	)
endif()

# Expose public includes to other
# subprojects through cache variable.
set(${This}_INCLUDE_DIRS 
	${PROJECT_SOURCE_DIR}/include
	${Boost_INCLUDE_DIRS}
	CACHE INTERNAL "${This}: Include Directories" FORCE
)