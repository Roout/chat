cmake_minimum_required (VERSION 3.12)

set(This mock_stream)
project(${This})

set(CMAKE_CXX_STANDARD 17)

set(headers)
set(sources)

list(APPEND headers
	"MockStream.hpp"
)
	
list(APPEND sources 
	"MockStream.cpp"
)

add_library(${This} STATIC ${sources} ${headers})
# add_executable(${This} ${sources} ${headers})

add_definitions(-DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB -D_WIN32_WINNT=0x0601)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    target_include_directories(${This} PUBLIC ${Boost_INCLUDE_DIRS})
endif()

# Expose public includes to other
# subprojects through cache variable.
set(${This}_INCLUDE_DIRS 
  ${PROJECT_SOURCE_DIR}/mock
  CACHE INTERNAL "${This}: Include Directories" FORCE
)