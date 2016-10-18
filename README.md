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

#include <co/adro/event_loop/event_loop_ev.h>
#include <co/adro/event_loop/watchers.h>
#include <co/adro/redis/redis_client.h>
#include <co/adro/redis/parser/base_resp_parser.h>
#include <co/adro/network/tcp_socket.h>
#include <co/adro/network/unix_socket.h>
#include <co/adro/network/tcp_server.h>

int main(int argc, char** args)
{
   co::adro::event_loop::EventLoopEV loop;
   co::adro::redis::RedisClient client(loop , new co::adro::network::UnixSocket(loop) , 1 );
   auto connect = [&](bool res) {
     if (!res) {
       std::cout << "didn't connect!" << std::endl;
       return;
     }

     client.Set("h1", "value1", [&](std::shared_ptr<co::adro::redis::parser::base_resp_parser> paresed) {
         std::cout << paresed->to_string() << std::endl;
         client.Get("h1", [&](std::shared_ptr<co::adro::redis::parser::base_resp_parser> p) {
             std::cout << p->to_string() << std::endl;
             client.Set("h2", "fooooo", [](std::shared_ptr<co::adro::redis::parser::base_resp_parser> p2) {
                 std::cout << p2->to_string() << std::endl;
               });
           });
       });

     };

//client.connect(connect, "127.0.0.1", 6379);
  client.Connect(connect, "/var/run/redis/redis.sock" );

  loop.Run();
  return 0;
}

```
