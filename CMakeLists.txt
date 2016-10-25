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
set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/source/co/adro)
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/includes)

include_directories(${PROJECT_INCLUDE_DIR})
include_directories("/usr/include/")
include_directories("/usr/local/include")

link_directories("/usr/lib")
link_directories("/usr/local/lib")

add_library(event_loop 
  ${PROJECT_SOURCE_DIR}/event_loop/event_loop_ev.cc
  ${PROJECT_SOURCE_DIR}/event_loop/watchers.cc
  )


add_library(network
  ${PROJECT_SOURCE_DIR}/network/async_socket.cc
  ${PROJECT_SOURCE_DIR}/network/tcp_server.cc
  ${PROJECT_SOURCE_DIR}/network/tcp_socket.cc
  ${PROJECT_SOURCE_DIR}/network/unix_socket.cc
  )

add_library(redis_client
  ${PROJECT_SOURCE_DIR}/redis/redis_client.cc
  ${PROJECT_SOURCE_DIR}/redis/connection.cc
  ${PROJECT_SOURCE_DIR}/redis/parser/array_parser.cc
  ${PROJECT_SOURCE_DIR}/redis/parser/bulk_string_parser.cc
  ${PROJECT_SOURCE_DIR}/redis/parser/error_parser.cc
  ${PROJECT_SOURCE_DIR}/redis/parser/number_parser.cc
  ${PROJECT_SOURCE_DIR}/redis/parser/redis_response.cc
  ${PROJECT_SOURCE_DIR}/redis/parser/simple_string_parser.cc
  )

target_link_libraries(event_loop ev)
target_link_libraries(network event_loop ev)
target_link_libraries(redis_client network event_loop ev)

install(TARGETS event_loop
  LIBRARY DESTINATION /usr/local/lib/
  ARCHIVE DESTINATION /usr/local/lib/)

install(TARGETS redis_client
  LIBRARY DESTINATION /usr/local/lib/
  ARCHIVE DESTINATION /usr/local/lib/
  )
install(TARGETS network
  LIBRARY DESTINATION /usr/local/lib/
  ARCHIVE DESTINATION /usr/local/lib/
  )

install(DIRECTORY ${PROJECT_INCLUDE_DIR}/ DESTINATION /usr/local/include)

add_executable (test ${CMAKE_SOURCE_DIR}/test/main.cpp)
target_link_libraries(test network redis_client event_loop ev)
