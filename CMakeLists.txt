cmake_minimum_required (VERSION 3.16)

project(chat-app)

enable_testing()

include("${CMAKE_SOURCE_DIR}/vendor/rapidjson.cmake")

# Download and unpack googletest at configure time
configure_file(vendor/googletest.txt.in googletest-download/CMakeLists.txt)
execute_process(
    COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download"
)
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build .
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/googletest-download"
)

# Prevent GoogleTest from overriding our compiler/linker options
# when building with Visual Studio
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This adds the following targets:
# gtest, gtest_main, gmock and gmock_main
add_subdirectory(
    ${CMAKE_BINARY_DIR}/googletest-src
    ${CMAKE_BINARY_DIR}/googletest-build
)

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(tests)