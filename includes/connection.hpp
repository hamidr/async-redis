#pragma once

#include <queue>
#include <functional>
#include <memory>
#include <tuple>

#include <parser/redis_response.h>
#include <network/tcp_socket.hpp>
#include <network/unix_socket.hpp>

namespace async_redis
{
  template<typename InputOutputHandler>
  class connection
  {
    using async_socket    = network::async_socket<InputOutputHandler>;
    using tcp_socket      = network::tcp_socket<InputOutputHandler>;
    using unix_socket     = network::unix_socket<InputOutputHandler>;

  public:
    using parser_t        = parser::redis_response::parser;
    using reply_cb_t      = std::function<void (parser_t)>;

    connection(InputOutputHandler &event_loop)
      : event_loop_(event_loop) {
    }

    void connect(typename async_socket::connect_handler_t handler, const std::string& ip, int port)
    {
      if (!socket_ || !socket_->is_valid())
        socket_ = std::make_unique<tcp_socket>(event_loop_);

      socket_->template async_connect<tcp_socket>(0, handler, ip, port);
    }

    void connect(typename async_socket::connect_handler_t handler, const std::string& path)
    {
      if (!socket_ || !socket_->is_valid())
        socket_ = std::make_unique<unix_socket>(event_loop_);

      socket_->template async_connect<unix_socket>(0, handler, path);
    }

    bool is_connected() const
    { return socket_ && socket_->is_connected(); }

    inline int pressure() const
    { return req_queue_.size(); }

    void disconnect() {
      socket_->close();
      decltype(req_queue_) free_me;
      free_me.swap(req_queue_);
    }

    bool pipelined_send(std::string&& pipelined_cmds, std::vector<reply_cb_t>&& callbacks)
    {
      return
        socket_->async_write(pipelined_cmds, [this, cbs = std::move(callbacks)](ssize_t sent_chunk_len) {
          if (sent_chunk_len == 0)
            return disconnect();

          if (!req_queue_.size() && cbs.size())
            socket_->async_read(data_, max_data_size, std::bind(&connection::reply_received, this, std::placeholders::_1));

          for(auto &&cb : cbs)
            req_queue_.emplace(std::move(cb), nullptr);
        });
    }

    bool send(const std::string&& command, const reply_cb_t& reply_cb)
    {
      bool read_it = !req_queue_.size();
      req_queue_.emplace(reply_cb, nullptr);

      return
        socket_->async_write(std::move(command), [this, read_it](ssize_t sent_chunk_len) {
          if (sent_chunk_len == 0)
            return disconnect();

        if (read_it)
          socket_->async_read(data_, max_data_size, std::bind(&connection::reply_received, this, std::placeholders::_1));
      });
    }

  protected:
    void reply_received(ssize_t len)
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
        acc += parser::redis_response::append_chunk(parser, data_ + acc, len - acc, is_finished);

        if (!is_finished)
          break;

        cb(parser);
        req_queue_.pop(); //free the resources
      }

      if (req_queue_.size())
        socket_->async_read(data_, max_data_size, std::bind(&connection::reply_received, this, std::placeholders::_1));
    }

  private:
    std::unique_ptr<async_socket> socket_;
    std::queue<std::tuple<reply_cb_t, parser_t>> req_queue_;

    InputOutputHandler& event_loop_;
    enum { max_data_size = 1024 };
    char data_[max_data_size];
  };
}
