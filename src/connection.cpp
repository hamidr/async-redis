#include "../includes/connection.hpp"

#include <queue>
#include <functional>
#include <memory>
#include <tuple>

#include <parser/base_resp_parser.h>
#include <iostream>

namespace async_redis
{

connection::connection(asio::io_context& io)
  : socket_(io)
{ }

void connection::connect(connect_handler_t handler, const std::string& ip, int port)
{
  asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(ip), port);

  socket_.async_connect(endpoint, [handler](const asio::error_code &ec) {
    // std::cout << ec << std::endl;
    handler(!ec);
  });
}

bool connection::is_connected() const
{
  return socket_.is_open();
}

void connection::disconnect()
{
  socket_.close();
  //TODO: check the policy! Should we free queue or retry again?
  decltype(req_queue_) free_me;
  free_me.swap(req_queue_);
}

bool connection::pipelined_send(std::string&& pipelined_cmds, std::vector<reply_cb_t>&& callbacks)
{
  if (!is_connected())
    return false;

  socket_.async_send(asio::buffer(pipelined_cmds.data(), pipelined_cmds.length()),
    [this, cbs = std::move(callbacks)](const asio::error_code &ec, size_t len)
    {
      if (ec)
        return disconnect();

      if (!req_queue_.size() && cbs.size())
        do_read();

      for(auto &&cb : cbs)
        req_queue_.emplace(std::move(cb), nullptr);
    }
  );

  return true;
}

bool connection::send(const std::string&& command, const reply_cb_t& reply_cb)
{
  if (!is_connected())
    return false;

  bool read_it = !req_queue_.size();
  req_queue_.emplace(reply_cb, nullptr);

  socket_.async_send(asio::buffer(command.data(), command.length()),
    [this, read_it](const asio::error_code &ec, size_t len)
    {
      // std::cout << ec << std::endl;
      if (ec)
        return disconnect();

      if (read_it)
        do_read();
    }
  );
  return true;
}

void connection::do_read()
{
  socket_.async_read_some(
    asio::buffer(data_, max_data_size),
    std::bind(
      &connection::reply_received,
      this,
      std::placeholders::_1,
      std::placeholders::_2
    )
  );
}

void connection::reply_received(const asio::error_code& ec, size_t len)
{
  // std::cout << ec << std::endl;
  if (ec)
    return disconnect();

  ssize_t acc = 0;
  while (acc < len && req_queue_.size())
  {
    auto& request = req_queue_.front();

    auto &cb = std::get<0>(request);
    auto &parser = std::get<1>(request);

    bool is_finished = false;
    acc += parser::base_resp_parser::append_chunk(parser, data_ + acc, len - acc, is_finished);

    if (!is_finished)
      break;

    cb(parser);
    req_queue_.pop(); //free the resources
  }

  if (req_queue_.size())
    do_read();
}

}
