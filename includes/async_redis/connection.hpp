#pragma once

#include <queue>
#include <functional>
#include <memory>
#include <tuple>

#include <async_redis/parser/base_resp_parser.h>
#include <libevpp/event_loop/event_loop_ev.h>
#include <libevpp/network/async_socket.hpp>

using namespace libevpp;

namespace async_redis
{
  class connection
  {
    using async_socket    = libevpp::network::async_socket;

  public:
    using parser_t        = parser::base_resp_parser::parser;
    using reply_cb_t      = std::function<void (parser_t&)>;

    connection(event_loop::event_loop_ev& event_loop);

    void connect(async_socket::connect_handler_t handler, const std::string& ip, int port);
    void connect(async_socket::connect_handler_t handler, const std::string& path);

    bool is_connected() const;
    void disconnect();
    bool pipelined_send(std::string&& pipelined_cmds, std::vector<reply_cb_t>&& callbacks);
    bool send(const std::string&& command, const reply_cb_t& reply_cb);

  private:
    void do_read();
    void reply_received(ssize_t len);

  private:
    std::unique_ptr<async_socket> socket_;
    std::queue<std::tuple<reply_cb_t, parser_t>> req_queue_;

    event_loop::event_loop_ev& event_loop_;
    enum { max_data_size = 1024 };
    char data_[max_data_size];
  };
}
