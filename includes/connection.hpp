#pragma once

#include <queue>
#include <functional>
#include <tuple>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#include <parser/base_resp_parser.h>

namespace async_redis
{
  class connection
  {
    connection(const connection&) = delete;
    connection& operator = (const connection&) = delete;

  public:
    using connect_handler_t    = std::function<void(bool)>;
    using parser_t        = parser::base_resp_parser::parser;
    using reply_cb_t      = std::function<void (parser_t)>;

    connection(asio::io_context&);

    void connect(connect_handler_t handler, const std::string& ip, int port);

    bool is_connected() const;
    void disconnect();
    bool pipelined_send(std::string&& pipelined_cmds, std::vector<reply_cb_t>&& callbacks);
    bool send(const std::string&& command, const reply_cb_t& reply_cb);

  private:
    void do_read();
    void reply_received(const asio::error_code& ec, size_t len);

  private:
    asio::ip::tcp::socket socket_;
    std::queue<std::tuple<reply_cb_t, parser_t>> req_queue_;

    enum { max_data_size = 1024 };
    char data_[max_data_size];
  };
}
