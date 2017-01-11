#include "../includes/async_redis/connection.hpp"

#include <queue>
#include <functional>
#include <memory>
#include <tuple>

#include <async_redis/parser/base_resp_parser.h>
#include <libevpp/network/tcp_socket.hpp>
#include <libevpp/network/unix_socket.hpp>


namespace async_redis
{

using tcp_socket      = network::tcp_socket;
using unix_socket     = network::unix_socket;

connection::connection(event_loop::event_loop_ev& event_loop)
  : event_loop_(event_loop) {
}

void connection::connect(async_socket::connect_handler_t handler, const std::string& ip, int port)
{
  if (!socket_ || !socket_->is_valid())
    socket_ = std::make_unique<tcp_socket>(event_loop_);

  static_cast<tcp_socket*>(socket_.get())->async_connect(ip, port, handler);
}

void connection::connect(async_socket::connect_handler_t handler, const std::string& path)
{
  if (!socket_ || !socket_->is_valid())
    socket_ = std::make_unique<unix_socket>(event_loop_);

  static_cast<unix_socket*>(socket_.get())->async_connect(path, handler);
}

bool connection::is_connected() const
{
  return socket_ && socket_->is_connected();
}

void connection::disconnect()
{
  socket_->close();
  //TODO: check the policy! Should we free queue or retry again?
  decltype(req_queue_) free_me;
  free_me.swap(req_queue_);
}

bool connection::pipelined_send(std::string&& pipelined_cmds, std::vector<reply_cb_t>&& callbacks)
{
  if (!is_connected())
    return false;

  return
    socket_->async_write(pipelined_cmds, [this, cbs = std::move(callbacks)](ssize_t sent_chunk_len) {
      if (sent_chunk_len == 0)
        return disconnect();

      if (!req_queue_.size() && cbs.size())
        do_read();

      for(auto &&cb : cbs)
        req_queue_.emplace(std::move(cb), nullptr);
    });
}

bool connection::send(const std::string&& command, const reply_cb_t& reply_cb)
{
  if (!is_connected())
    return false;

  bool read_it = !req_queue_.size();
  req_queue_.emplace(reply_cb, nullptr);

  return
    socket_->async_write(std::move(command), [this, read_it](ssize_t sent_chunk_len) {
      if (sent_chunk_len == 0)
        return disconnect();

    if (read_it)
      do_read();
  });
}

void connection::do_read()
{
  socket_->async_read(data_, max_data_size, std::bind(&connection::reply_received, this, std::placeholders::_1));
}

void connection::reply_received(ssize_t len)
{
  if (len == 0)
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
