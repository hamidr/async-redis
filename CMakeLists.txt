cmake_minimum_required (VERSION 2.8)

project (async-redis)

set(DEBUG_MODE ON CACHE BOOL "Debug mode") 
set(LIB_INSTALL_DIR "/ust/local/lib/" CACHE PATH "")

if(NOT DEBUG_MODE)
  ##SET RELEASE MODE FLAGS
else()
  ##SET DEUBG MODE FLAGS
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/source)
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/includes)

include_directories(${PROJECT_INCLUDE_DIR})
include_directories("/usr/include/")
include_directories("/usr/local/include")

link_directories("/usr/lib")
link_directories("/usr/local/lib")

add_library(event_loop 
  ${PROJECT_SOURCE_DIR}/co/adro/event_loop/event_loop_ev.cc
  ${PROJECT_SOURCE_DIR}/co/adro/event_loop/watchers.cc
  )

add_library(parser 
  ${PROJECT_SOURCE_DIR}/parser/array_parser.cpp
  ${PROJECT_SOURCE_DIR}/parser/bulk_string_parser.cpp
  ${PROJECT_SOURCE_DIR}/parser/error_parser.cpp
  ${PROJECT_SOURCE_DIR}/parser/number_parser.cpp
  ${PROJECT_SOURCE_DIR}/parser/redis_response.cpp
  ${PROJECT_SOURCE_DIR}/parser/simple_string_parser.cpp)

target_link_libraries(event_loop ev)

install(TARGETS event_loop
  LIBRARY DESTINATION /usr/local/lib/
  ARCHIVE DESTINATION /usr/local/lib/)

install(TARGETS parser
  LIBRARY DESTINATION /usr/local/lib/
  ARCHIVE DESTINATION /usr/local/lib/
  )

install(DIRECTORY ${PROJECT_INCLUDE_DIR}/ DESTINATION /usr/local/include)

add_executable (test ${CMAKE_SOURCE_DIR}/test/main.cpp)
target_link_libraries(test parser event_loop ev)
