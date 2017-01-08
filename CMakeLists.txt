cmake_minimum_required (VERSION 2.8)

project (async-redis)

set(DEBUG_MODE ON CACHE BOOL "Debug mode") 
set(LIB_INSTALL_DIR "/ust/local/lib/" CACHE PATH "")

if(NOT DEBUG_MODE)
  ##SET RELEASE MODE FLAGS
  SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
  SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )
else()
  ##SET DEUBG MODE FLAGS
  SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS} -g" )
  SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS} -g" )
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src)
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/includes)

include_directories(${PROJECT_INCLUDE_DIR})
include_directories("/usr/include/")
include_directories(${CMAKE_SOURCE_DIR}/libs/asio/asio/include/)

link_directories("/usr/lib")
link_directories("/usr/local/lib")


add_library(parser
  ${PROJECT_SOURCE_DIR}/parser/base_resp_parser.cpp
  ${PROJECT_SOURCE_DIR}/parser/array_parser.cpp
  ${PROJECT_SOURCE_DIR}/parser/bulk_string_parser.cpp
  ${PROJECT_SOURCE_DIR}/parser/error_parser.cpp
  ${PROJECT_SOURCE_DIR}/parser/number_parser.cpp
  ${PROJECT_SOURCE_DIR}/parser/simple_string_parser.cpp)

add_library(async_redis
  ${PROJECT_SOURCE_DIR}/connection.cpp
  ${PROJECT_SOURCE_DIR}/monitor.cpp
  ${PROJECT_SOURCE_DIR}/sentinel.cpp
  ${PROJECT_SOURCE_DIR}/redis_client.cpp)


## Compiler flags
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-O2")        ## Optimize
    set(CMAKE_EXE_LINKER_FLAGS "-s")  ## Strip binary
endif()

target_link_libraries(async_redis parser pthread)

install(TARGETS async_redis
  LIBRARY DESTINATION /usr/local/lib/
  ARCHIVE DESTINATION /usr/local/lib/
  )

install(TARGETS parser
  LIBRARY DESTINATION /usr/local/lib/
  ARCHIVE DESTINATION /usr/local/lib/
  )


install(DIRECTORY ${PROJECT_INCLUDE_DIR}/ DESTINATION /usr/local/include)

add_executable (a1.out ${CMAKE_SOURCE_DIR}/test/main.cpp)
target_link_libraries(a1.out async_redis)
