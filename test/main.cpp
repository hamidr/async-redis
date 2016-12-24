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
  //if (argc != 2)
  // return 0;

  // using redis_client_t = async_redis::redis_impl::redis_client<decltype(loop), async_redis::network::unix_socket<decltype(loop)>>;
  using redis_client_t = async_redis::redis_impl::redis_client<decltype(loop), async_redis::network::tcp_socket<decltype(loop)>>;

   async_redis::event_loop::event_loop_ev loop;
   using sentinel_t = async_redis::redis_sentinel<loop, async_redis::network::tcp_socket<decltype(loop)>>;
   sentinel_t sentinel;

   std::vector<std::pair<std::string, int>> sentinel_addresses = {
     {"192.168.2.43", 26379},
     {"192.168.2.43", 26378}
   };


   using parser_t = typename sentinel_t::redis_client_t::parser_t;

   sentinel.connect(sentinel_addresses, [&](bool res) {

    auto &client = sentinel.get_master();
    auto &slave_client = sentinel.get_slave();

     if (!res) {
       //std::cout << "didn't connect!" << std::endl << std::endl;
       return;
     }
     client.pipeline_on();

     client.select(0, [](parser_t){});

     client.set("h2", "wwww2", [&](parser_t p) {
         std::cout << "set h2 " << p->to_string() << std::endl << std::endl;
       });
     client.ping([&](parser_t p) {
         std::cout << "ping "<< p->to_string() << std::endl << std::endl;
       });
     client.get("h3", [&](parser_t p) {
         std::cout << "get h3 "<< p->to_string() << std::endl << std::endl;
       });
     client.ping([&](parser_t p) {
         std::cout << "ping "<<p->to_string() << std::endl << std::endl;
       });

     client.pipeline_off().commit_pipeline();
     return;


     client.set("h4", "wwww", [&](parser_t paresed) {
         std::cout << "h4 www "<<paresed->to_string() << std::endl << std::endl;
         client.get("h5", [&](parser_t p) {
             std::cout << "get h5 " <<p->to_string() << std::endl << std::endl;

             client.set("wtff", "hello", [&](parser_t paresed) {
                 client.get("wtff", [](parser_t p2) {
                     std::cout << p2->to_string() << std::endl << std::endl;
                   });

                 client.get("h1", [](parser_t p2) {
                     std::cout << p2->to_string() << std::endl << std::endl;
                   });

                 client.get("wtff", [&](parser_t p2) {
                     std::cout << p2->to_string() << std::endl << std::endl;

                     client.get("h1", [](parser_t p2) {
                         std::cout <<"h1" <<p2->to_string() << std::endl << std::endl;
                       });
                   });
               });
           });
       });


   for(int i = 0; i< 2; ++i)
   client.get("hamid", [&](parser_t parsed) {
       //std::cout <<"get hamid =>" << parsed->to_string() << std::endl << std::endl;

       for(int i = 0; i< 10; ++i)
         client.get("hamid", [&](parser_t parsed) {
             //std::cout <<"get hamid =>" << parsed->to_string()<< std::endl << std::endl;

             for(int i = 0; i< 10; ++i)
               client.get("hamid", [&](parser_t parsed) {
                   //std::cout <<"get hamid =>" << parsed->to_string()<< std::endl << std::endl;
                   for(int i = 0; i< 10; ++i)
                     client.get("hamid", [&](parser_t parsed) {
                         //std::cout <<"get hamid =>" << parsed->to_string()<< std::endl << std::endl;
                       });
                 });
           });
     });


   // for(int i = 0; i< 20; ++i)
   //   client.set(std::string("key") + std::to_string(i), std::to_string(i), [&](parser_t parsed) {
   //     //std::cout <<"set key2 value2 =>" << parsed->to_string() << std::endl << std::endl;
   //       for(int i = 0; i< 10; ++i)
   //         client.get("hamid", [&](parser_t parsed) {
   //             //std::cout <<"get hamid =>" << parsed->to_string()<< std::endl << std::endl;
   //           });
   //   });

   client.keys("*", [](parser_t parsed) {
       //std::cout <<"keys " << parsed->to_string() << std::endl << std::endl;
     });


   client.keys("*", [](parser_t parsed) {
       //std::cout <<"keys " << parsed->to_string() << std::endl << std::endl;
     });

   client.hgetall("myhash", [](parser_t parsed) {
       //std::cout <<"hgetall hello " << parsed->to_string() << std::endl << std::endl;
     });

   //TODO:WTF?
   // for(int i = 0; i< 100; ++i)
   // client.set("n1"+i, "1", [](std::shared_ptr<IO::base_resp_parser> parsed) {
   //     // //std::cout<< "*" <<"set n1 " << parsed->to_string()  <<"*"<< std::endl << std::endl;
   //   });

   client.get("n1", [](parser_t parsed) {
       //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;
     });

   client.incr("n1", [](parser_t parsed) {
       //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;
     });

   client.incr("n1", [&](parser_t parsed) {
       //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;

       for(int i = 0; i< 100; ++i)
         client.incr("n1", [](parser_t parsed) {
             //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;
           });
     });

   client.decr("n1", [](parser_t parsed) {
       // //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;
     });

   // for(int i = 0; i< 100; ++i)
   // client.err([](std::shared_ptr<IO::base_resp_parser> parsed) {
   //     //std::cout <<"err =>" << parsed->to_string()<< std::endl << std::endl;
   //   });

   for(int i = 0; i< 600; ++i)
   client.get("n1", [](parser_t parsed) {
       // //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;
     });

   for(int i = 0; i< 100; ++i)
   client.hmget("myhash", {"field2"}, [](parser_t parsed) {
       // //std::cout <<"hmget myhash =>" << parsed->to_string()<< std::endl << std::endl << std::endl << std::endl;
     });


   for(int i = 0; i< 100; ++i)
   client.hmget("myhash", {"field2", "field1"}, [](parser_t parsed) {
       //std::cout <<"hmget myhash =>" << parsed->to_string()<< std::endl << std::endl << std::endl << std::endl;
     });

     };

   client.connect(connect, "127.0.0.1", 6379);
   // client.connect(connect, "/tmp/redis.sock");

   loop.run();

  return 0;
}
