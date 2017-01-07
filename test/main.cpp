#include <event_loop/event_loop_ev.h>

#include <redis_client.hpp>
#include <parser/base_resp_parser.h>
#include <network/tcp_socket.hpp>
#include <network/unix_socket.hpp>
#include <monitor.hpp>
#include <sentinel.hpp>
#include "../examples/tcp_server.hpp"


typedef async_redis::event_loop::event_loop_ev event_loop_ev;

struct monitor_test
{
  using parser         = async_redis::parser::base_resp_parser;
  using monitor_t      = async_redis::monitor;
  using parsed_t       = parser::parser;
  using State          = monitor_t::EventState;

  using redis_client_t = async_redis::redis_client;

  monitor_test(event_loop_ev &loop, int n = 100)
    : my_monitor(std::make_unique<monitor_t>(loop)),
      my_redis(std::make_unique<redis_client_t>(loop, 2)),
      n_ping(n)
  {
    start();
  }

  void start() {
    my_redis->connect(std::bind(&monitor_test::check_redis_connected, this, std::placeholders::_1), "127.0.0.1", 6379);
  }

  void check_redis_connected(bool status)
  {
    if (status) {
      std::cout << "RedisClient connected! \n";
      my_monitor->connect(std::bind(&monitor_test::check_monitor_connected, this, std::placeholders::_1), "127.0.0.1", 6379);
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
  using sentinel_t = async_redis::sentinel;
  using parser_t   = sentinel_t::parser_t;

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
  event_loop_ev loop;

  monitor_test monitor_mock(loop, 100000);
  // async_redis::tcp_server::tcp_server server(loop);
  // server.listen(9090);


  loop.run();
  return 0;
}
