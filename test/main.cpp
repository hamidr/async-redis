#include <memory>
#include <iostream>

#include <event_loop/event_loop_ev.h>
#include <redis_client.hpp>
#include <parser/base_resp_parser.h>
#include <network/tcp_socket.hpp>
#include <network/unix_socket.hpp>
#include <monitor.hpp>
#include <sentinel.hpp>


typedef async_redis::event_loop::event_loop_ev event_loop_ev;

struct monitor_test
{
  using tcp_socket_t   = async_redis::network::tcp_socket<event_loop_ev>;
  using parser         = async_redis::parser::redis_response;
  using monitor_t      = async_redis::redis_impl::monitor<event_loop_ev, tcp_socket_t, parser>;
  using parsed_t       = typename parser::parser;
  using State          = typename monitor_t::EventState;

  using redis_client_t = async_redis::redis_impl::redis_client<event_loop_ev, tcp_socket_t>;

  monitor_test(event_loop_ev &loop, int n = 100)
    : my_monitor(std::make_unique<monitor_t>(loop)),
      my_redis(std::make_unique<redis_client_t>(loop, 2)),
      n_ping(n)
  {
    start();
  }

  void start() {
    my_redis->connect(std::bind(&monitor_test::check_redis_connected, this, std::placeholders::_1), "10.42.0.140", 80);
  }

  void check_redis_connected(bool status)
  {
    if (status) {
      std::cout << "RedisClient connected! \n";
      my_monitor->connect(std::bind(&monitor_test::check_monitor_connected, this, std::placeholders::_1), "10.42.0.140", 80);
    } else {
      std::cout << "REDIS DIDNT CONNECT!" << std::endl;
    }
  }

  void check_monitor_connected(bool status) {
    if (status) {
      std::cout << "Monitor connected!" << std::endl;
      my_monitor->subscribe({"ping"}, std::bind(&monitor_test::watch_hello_msgs, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    } else {
      std::cout << "MONITOR DIDNT CONNECT!" << std::endl;
      my_redis->disconnect();
    }
  }


  void watch_hello_msgs(const std::string& channel, parsed_t event, State state)
  {
    switch(state)
    {
    case State::Sub:
      std::cout << "watch StartResult" << std::endl;
      send_ping(0);
      break;

    case State::Stream:
      std::cout << "watch EventStream" << std::endl;
      if (play_with_event(event))
        return;
      my_monitor->unsubscribe({"ping"}, std::bind(&monitor_test::watch_hello_msgs, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

      break;
    case State::Disconnected:
      std::cout << "Are we fucked" << std::endl;
      my_redis->disconnect();
      start();
      break;

    case State::Unsub:
      std::cout << "StopResult" << std::endl;
      break;
    }
  }

  void send_ping(long n)
  {
    std::cout << "Pinging" << std::endl;
    my_redis->publish("ping", std::to_string(n),
      [](parsed_t res)
      {
        std::cout << "Pinged " << res->to_string() << " connections."  << std::endl;
      }
    );
  }

  bool play_with_event(const parsed_t& event)
  {
    std::cout << "Ponged by " << event->to_string() << std::endl;
    int n = 0;

    long x = std::stol(event->to_string());
    if (x != n_ping) {
      send_ping(x+1);
      return true;
    }
    return false;
  }

private:
  std::unique_ptr<monitor_t> my_monitor;
  std::unique_ptr<redis_client_t> my_redis;
  const long n_ping;
};


struct sentinel_test
{
  using sentinel_t = async_redis::redis_impl::sentinel<event_loop_ev>;
  using parser_t   = typename sentinel_t::parser_t;

  sentinel_test(event_loop_ev& ev)
    : io_(ev),
      my_sen1(std::make_unique<sentinel_t>(ev))
  {
    my_sen1->connect("10.42.0.140", 8080, std::bind(&sentinel_test::check_connected, this, std::placeholders::_1));
  }

  void check_connected(bool result)
  {
    if (result) {
      my_sen1->watch_master_change(std::bind(&sentinel_test::master_changed, this, std::placeholders::_1, std::placeholders::_2));
      return;
    }

    std::cout << "sentinel not connected!" << std::endl;
  }

  void master_changed(const std::vector<std::string>&& info, sentinel_t::SentinelState state)
  {
    using x = sentinel_t::SentinelState;

    switch(state) {
    case x::Disconnected:
      std::cout << "disconnected" << std::endl;
      break;
    case x::Watching:
      std::cout << "watching" << std::endl;
      break;
    }
    for(auto &w : info)
      std::cout << w << std::endl;

  }

  void forced_failover(parser_t value)
  {
    value->print();
  }

private:
  std::unique_ptr<sentinel_t> my_sen1;
  event_loop_ev &io_;
};

int main(int argc, char** args)
{
  //if (argc != 2)
  // return 0;

  event_loop_ev loop;

  monitor_test monitor_mock(loop, 10000);
  // sentinel_test sentinel_mock(loop);

  // using unix_socket_t = async_redis::network::unix_socket<event_loop_ev>;
  // using tcp_socket_t = async_redis::network::tcp_socket<event_loop_ev>;

  // using redis_client_t = async_redis::redis_impl::redis_client<decltype(loop), unix_socket_t>;
  // // using redis_client_t = async_redis::redis_impl::redis_client<decltype(loop), tcp_socket>;

  // using monitor_t = async_redis::redis_impl::monitor<event_loop_ev, unix_socket_t, async_redis::parser::redis_response>;

  // using parser_t = async_redis::parser::redis_response::parser;

  // using sentinel_t = async_redis::redis_impl::sentinel<event_loop_ev>;
  // monitor_t m(loop);
  // sentinel_t sen(loop);

  // redis_client_t client(loop);

  // // using sentinel_t = async_redis::redis_sentinel<loop, async_redis::network::tcp_socket<decltype(loop)>>;
  // // sentinel_t sentinel;



  //  m.subscribe({"hello"}, [](parser_t value, monitor_t::State state) -> bool {
  //      return true;
  //    });

  //  sen.connect("10.10.1.2", 6379, [&sen](bool res) {
  //      if (res)
  //        return;

  //      sen.ping([](parser_t value) {
  //        });

  //      sen.master_addr_by_name("test", [](const std::string& ip, int port, bool res)
  //                              {
                                 
  //                              });

  //      sen.watch_master_change([](const std::string& ip, int port) -> bool
  //                              {
  //                                return true;
  //                              });

  //    });


  //  std::vector<std::pair<std::string, int>> sentinel_addresses = {
  //    {"192.168.2.43", 26379},
  //    {"192.168.2.43", 26378}
  //  };

  //  using redis_sentinel_t = async_redis::redis_impl::redis_sentinel<event_loop_ev>;

  //  redis_sentinel_t rs(loop, "redis-cluster");
  //  rs.connect(sentinel_addresses, [](const std::string& ip, int port, typename redis_sentinel_t::State state ) {

  //    });

   // using parser_t = typename sentinel_t::redis_client_t::parser_t;

   // sentinel.connect(sentinel_addresses, [&](bool res) {

   //  auto &client = sentinel.get_master();
   //  auto &slave_client = sentinel.get_slave();

   //   if (!res) {
   //     //std::cout << "didn't connect!" << std::endl << std::endl;
   //     return;
   //   }
   //   client.pipeline_on();

   //   client.select(0, [](parser_t){});

   //   client.set("h2", "wwww2", [&](parser_t p) {
   //       std::cout << "set h2 " << p->to_string() << std::endl << std::endl;
   //     });
   //   client.ping([&](parser_t p) {
   //       std::cout << "ping "<< p->to_string() << std::endl << std::endl;
   //     });
   //   client.get("h3", [&](parser_t p) {
   //       std::cout << "get h3 "<< p->to_string() << std::endl << std::endl;
   //     });
   //   client.ping([&](parser_t p) {
   //       std::cout << "ping "<<p->to_string() << std::endl << std::endl;
   //     });

   //   client.pipeline_off().commit_pipeline();
   //   return;


   //   client.set("h4", "wwww", [&](parser_t paresed) {
   //       std::cout << "h4 www "<<paresed->to_string() << std::endl << std::endl;
   //       client.get("h5", [&](parser_t p) {
   //           std::cout << "get h5 " <<p->to_string() << std::endl << std::endl;

   //           client.set("wtff", "hello", [&](parser_t paresed) {
   //               client.get("wtff", [](parser_t p2) {
   //                   std::cout << p2->to_string() << std::endl << std::endl;
   //                 });

   //               client.get("h1", [](parser_t p2) {
   //                   std::cout << p2->to_string() << std::endl << std::endl;
   //                 });

   //               client.get("wtff", [&](parser_t p2) {
   //                   std::cout << p2->to_string() << std::endl << std::endl;

   //                   client.get("h1", [](parser_t p2) {
   //                       std::cout <<"h1" <<p2->to_string() << std::endl << std::endl;
   //                     });
   //                 });
   //             });
   //         });
   //     });


   // for(int i = 0; i< 2; ++i)
   // client.get("hamid", [&](parser_t parsed) {
   //     //std::cout <<"get hamid =>" << parsed->to_string() << std::endl << std::endl;

   //     for(int i = 0; i< 10; ++i)
   //       client.get("hamid", [&](parser_t parsed) {
   //           //std::cout <<"get hamid =>" << parsed->to_string()<< std::endl << std::endl;

   //           for(int i = 0; i< 10; ++i)
   //             client.get("hamid", [&](parser_t parsed) {
   //                 //std::cout <<"get hamid =>" << parsed->to_string()<< std::endl << std::endl;
   //                 for(int i = 0; i< 10; ++i)
   //                   client.get("hamid", [&](parser_t parsed) {
   //                       //std::cout <<"get hamid =>" << parsed->to_string()<< std::endl << std::endl;
   //                     });
   //               });
   //         });
   //   });


   // // for(int i = 0; i< 20; ++i)
   // //   client.set(std::string("key") + std::to_string(i), std::to_string(i), [&](parser_t parsed) {
   // //     //std::cout <<"set key2 value2 =>" << parsed->to_string() << std::endl << std::endl;
   // //       for(int i = 0; i< 10; ++i)
   // //         client.get("hamid", [&](parser_t parsed) {
   // //             //std::cout <<"get hamid =>" << parsed->to_string()<< std::endl << std::endl;
   // //           });
   // //   });

   // client.keys("*", [](parser_t parsed) {
   //     //std::cout <<"keys " << parsed->to_string() << std::endl << std::endl;
   //   });


   // client.keys("*", [](parser_t parsed) {
   //     //std::cout <<"keys " << parsed->to_string() << std::endl << std::endl;
   //   });

   // client.hgetall("myhash", [](parser_t parsed) {
   //     //std::cout <<"hgetall hello " << parsed->to_string() << std::endl << std::endl;
   //   });

   // //TODO:WTF?
   // // for(int i = 0; i< 100; ++i)
   // // client.set("n1"+i, "1", [](std::shared_ptr<IO::base_resp_parser> parsed) {
   // //     // //std::cout<< "*" <<"set n1 " << parsed->to_string()  <<"*"<< std::endl << std::endl;
   // //   });

   // client.get("n1", [](parser_t parsed) {
   //     //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;
   //   });

   // client.incr("n1", [](parser_t parsed) {
   //     //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;
   //   });

   // client.incr("n1", [&](parser_t parsed) {
   //     //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;

   //     for(int i = 0; i< 100; ++i)
   //       client.incr("n1", [](parser_t parsed) {
   //           //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;
   //         });
   //   });

   // client.decr("n1", [](parser_t parsed) {
   //     // //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;
   //   });

   // // for(int i = 0; i< 100; ++i)
   // // client.err([](std::shared_ptr<IO::base_resp_parser> parsed) {
   // //     //std::cout <<"err =>" << parsed->to_string()<< std::endl << std::endl;
   // //   });

   // for(int i = 0; i< 600; ++i)
   // client.get("n1", [](parser_t parsed) {
   //     // //std::cout <<"get n1 =>" << parsed->to_string()<< std::endl << std::endl;
   //   });

   // for(int i = 0; i< 100; ++i)
   // client.hmget("myhash", {"field2"}, [](parser_t parsed) {
   //     // //std::cout <<"hmget myhash =>" << parsed->to_string()<< std::endl << std::endl << std::endl << std::endl;
   //   });


   // for(int i = 0; i< 100; ++i)
   // client.hmget("myhash", {"field2", "field1"}, [](parser_t parsed) {
   //     //std::cout <<"hmget myhash =>" << parsed->to_string()<< std::endl << std::endl << std::endl << std::endl;
   //   });

   //   };

   // client.connect(connect, "127.0.0.1", 6379);
   // // client.connect(connect, "/tmp/redis.sock");

  // try {
    loop.run();
  // } catch (std::exception &e) {
    // std::cout << e.what() << std::endl;
  // }


  return 0;
}
