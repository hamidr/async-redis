# async-redis
Another redis library written in c++1y with using libev(and an interface to support other event-loop driven designs).

###[Still under heavy development]
Right now it's just a redis library but I'm gonna separate the IO/Network package out to use it for other things as well.

##INSTALL
Create a directory in root for generate CMake output file.
```
mkdir build 
cd build
```
Call cmake and pass project root directory as CMakeList path then call make install with sudo permission.
```
cmake ..
sudo make install
```
You can use event_loop and parser as link library for your project and pass /usr/local/include as include directory.
##Show me an example:

```C++
#include <memory>
#include <iostream>

#include <event_loop/event_loop_ev.h>
#include <redis_client.hpp>
#include <parser/base_resp_parser.h>
#include <network/tcp_socket.hpp>
#include <network/unix_socket.hpp>

int main(int argc, char** args)
{
   async_redis::event_loop::event_loop_ev loop;
   //If you want tcp socket
   // using redis_client_t = async_redis::redis_impl::redis_client<decltype(loop), async_redis::network::tcp_socket>;
   using redis_client_t = async_redis::redis_impl::redis_client<decltype(loop), async_redis::network::unix_socket>;
   using parser_t = typename redis_client_t::parser_t;

   //a tcp client with 4 connections
   auto client_ptr = std::make_unique<redis_client_t>(loop, 4);

   auto connect = [&](bool res)
   {

      //all 4 connections should be available for "res" to be True
      if (!res) {
        std::cout << "didn't connect!" << std::endl;
        return;
      }

      //all commands are defined as methods
      client.get("hamid", [&](parser_t parsed) {
        std::cout <<"get hamid =>" << parsed->to_string()<< std::endl;
      });
   };

   //if you want tcp_socket
   // client.connect(connect, "127.0.0.1", 6379);
   client.connect(connect, "/tmp/redis.sock");

   loop.run();

  return 0;
}

```
