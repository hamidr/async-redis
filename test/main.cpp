#include <memory>
#include <iostream>

#include <event_loop/event_loop_ev.h>
#include <redis_client.hpp>
#include <parser/base_resp_parser.h>
#include <network/tcp_socket.hpp>
#include <network/unix_socket.hpp>
#include <network/tcp_server.hpp>

int main(int argc, char** args)
{
   async_redis::event_loop::event_loop_ev loop;
   using redis_client_t = async_redis::redis_impl::redis_client<decltype(loop), async_redis::network::unix_socket<decltype(loop)>>;
   // using redis_client_t = async_redis::redis_impl::redis_client<decltype(loop), async_redis::network::tcp_socket<decltype(loop)>>;

   std::unique_ptr<redis_client_t> client_ptr;


   async_redis::tcp_server::tcp_server<decltype(loop), async_redis::tcp_server::test_parser> server(loop);
   // server.listen(9080);

   try {
     client_ptr = std::make_unique<redis_client_t>(loop, 1);
   }
   catch(...) {
     std::cerr << "We are so fucked! So fucking fucked!" << std::endl;
     return -1;
   }

   redis_client_t& client = *client_ptr;

   using parser_t = typename redis_client_t::parser_t;

   auto connect = [&](bool res) {

     if (!res) {
       std::cout << "didn't connect!" << std::endl;
       return;
     }

     client.set("h1", "value1", [&](parser_t paresed) {
         std::cout << paresed->to_string() << std::endl;
         client.get("h1", [&](parser_t p) {
             std::cout << p->to_string() << std::endl;
             client.set("h2", "fooooo", [](parser_t p2) {
                 std::cout << p2->to_string() << std::endl;
               });
           });
       });


   for(int i = 0; i< 2; ++i)
   client.get("hamid", [&](parser_t parsed) {
       // std::cout <<"get hamid =>" << parsed->to_string() << std::endl;

       for(int i = 0; i< 10; ++i)
         client.get("hamid", [&](parser_t parsed) {
             // std::cout <<"get hamid =>" << parsed->to_string()<< std::endl;

             for(int i = 0; i< 10; ++i)
               client.get("hamid", [&](parser_t parsed) {
                   // std::cout <<"get hamid =>" << parsed->to_string()<< std::endl;
                   for(int i = 0; i< 10; ++i)
                     client.get("hamid", [&](parser_t parsed) {
                         // std::cout <<"get hamid =>" << parsed->to_string()<< std::endl;
                       });
                 });
           });
     });

   for(int i = 0; i< 20; ++i)
     client.set(std::string("key") + std::to_string(i), std::to_string(i), [&](parser_t parsed) {
       // std::cout <<"set key2 value2 =>" << parsed->to_string() << std::endl;
         for(int i = 0; i< 10; ++i)
           client.get("hamid", [&](parser_t parsed) {
               // std::cout <<"get hamid =>" << parsed->to_string()<< std::endl;
             });
     });

   client.keys("*", [](parser_t parsed) {
       // std::cout <<"keys " << parsed->to_string() << std::endl;
     });


   client.keys("*", [](parser_t parsed) {
       // std::cout <<"keys " << parsed->to_string() << std::endl;
     });

   client.hgetall("myhash", [](parser_t parsed) {
       // std::cout <<"hgetall hello " << parsed->to_string() << std::endl;
     });

   //TODO:WTF?
   // for(int i = 0; i< 100; ++i)
   // client.set("n1"+i, "1", [](std::shared_ptr<IO::base_resp_parser> parsed) {
   //     // std::cout<< "*" <<"set n1 " << parsed->to_string()  <<"*"<< std::endl;
   //   });

   client.get("n1", [](parser_t parsed) {
       // std::cout <<"get n1 =>" << parsed->to_string()<< std::endl;
     });

   client.incr("n1", [](parser_t parsed) {
       // std::cout <<"get n1 =>" << parsed->to_string()<< std::endl;
     });

   client.incr("n1", [&](parser_t parsed) {
       // std::cout <<"get n1 =>" << parsed->to_string()<< std::endl;

       for(int i = 0; i< 100; ++i)
         client.incr("n1", [](parser_t parsed) {
             // std::cout <<"get n1 =>" << parsed->to_string()<< std::endl;
           });
     });

   client.decr("n1", [](parser_t parsed) {
       // std::cout <<"get n1 =>" << parsed->to_string()<< std::endl;
     });

   // for(int i = 0; i< 100; ++i)
   // client.err([](std::shared_ptr<IO::base_resp_parser> parsed) {
   //     std::cout <<"err =>" << parsed->to_string()<< std::endl;
   //   });

   for(int i = 0; i< 100; ++i)
   client.get("n1", [](parser_t parsed) {
       // std::cout <<"get n1 =>" << parsed->to_string()<< std::endl;
     });

   for(int i = 0; i< 100; ++i)
   client.hmget("myhash", {"field2"}, [](parser_t parsed) {
       // std::cout <<"hmget myhash =>" << parsed->to_string()<< std::endl;
     });


   for(int i = 0; i< 100; ++i)
   client.hmget("myhash", {"field2", "field1"}, [](parser_t parsed) {
       // std::cout <<"hmget myhash =>" << parsed->to_string()<< std::endl;
     });

     };

   // client.connect(connect, "127.0.0.1", 6379);
   client.connect(connect, "/tmp/redis.sock");

   loop.run();

  return 0;
}
