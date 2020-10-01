cmake_minimum_required (VERSION 3.16)

project(chat-app)

include(ExternalProject)
include("${CMAKE_SOURCE_DIR}/vendor/rapidjson.cmake")

add_subdirectory(googletest)
add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(tests)