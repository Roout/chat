cmake_minimum_required (VERSION 3.16)

set(This client)
set(CMAKE_CXX_STANDARD 17)

set(headers)
set(sources)

list(APPEND headers
  "Client.hpp"
  "Connection.hpp"
)

list(APPEND sources 
  "Client.cpp"
  "main.cpp"
)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# debug message
message(STATUS "Client debug message: ")
if(${OPENSSL_FOUND})
  message(STATUS "OPENSSL_FOUND is true")
  message(STATUS "OPENSSL_VERSION ${OPENSSL_VERSION}")
  # message(STATUS "OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR}")
  # message(STATUS "OPENSSL_CRYPTO_LIBRARY ${OPENSSL_CRYPTO_LIBRARY}")
  # message(STATUS "OPENSSL_CRYPTO_LIBRARIES ${OPENSSL_CRYPTO_LIBRARIES}")
  # message(STATUS "OPENSSL_SSL_LIBRARY ${OPENSSL_SSL_LIBRARY}")
  # message(STATUS "OPENSSL_SSL_LIBRARIES ${OPENSSL_SSL_LIBRARIES}")
  # message(STATUS "OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES}")
  # message(STATUS "OPENSSL_APPLINK_SOURCE ${OPENSSL_APPLINK_SOURCE}")
else()
  message(STATUS "OPENSSL_FOUND is false")
endif()

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

add_definitions(
  -DBOOST_DATE_TIME_NO_LIB 
  -DBOOST_REGEX_NO_LIB 
  -D_WIN32_WINNT=0x0601
  -D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING
  -DRAPIDJSON_NOMEMBERITERATORCLASS
)

foreach(ctarget ${client_targets})
  target_include_directories(${ctarget} 
    PRIVATE ${RAPIDJSON_INCLUDE_DIR}
    PRIVATE ${common_INCLUDE_DIRS}
    PRIVATE ${OPENSSL_INCLUDE_DIR}
  )
  target_link_libraries(${ctarget}
    PRIVATE common
    PRIVATE ${CMAKE_THREAD_LIBS_INIT}
    PRIVATE OpenSSL::SSL
    PRIVATE OpenSSL::Crypto
  )
endforeach()

# Expose public includes to other
# subprojects through cache variable.
set(${This}_INCLUDE_DIRS 
  ${PROJECT_SOURCE_DIR}/client
  CACHE INTERNAL "${This}: Include Directories" FORCE
)

target_compile_options(${This} PRIVATE
  # $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:Clang>:-Wall -Werror -Wextra>>
  # $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:GNU>:-Wall -Werror -Wextra>>
  $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:MSVC>:/W3>>
)
