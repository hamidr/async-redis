#pragma once

#include <string>
#include <memory>
#include <functional>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace async_redis {
namespace tcp_server {

  template<typename InputOutputHandler>
  class tcp_server
  {
  public:
    using tcp_socket   = async_redis::network::tcp_socket<InputOutputHandler>;

    tcp_server(InputOutputHandler &event_loop)
      : loop_(event_loop) {
      listener_ = std::make_shared<tcp_socket>(event_loop);
    }

    void listen(int port) {
      if (!listener_->bind("127.0.0.1", port) || !listener_->listen())
        throw;

      auto receiver = std::bind(&tcp_server::accept, this, std::placeholders::_1);
      listener_->template async_accept<tcp_socket>(receiver);
    }

    void accept(std::shared_ptr<tcp_socket> socket) {
      auto receiver = std::bind(&tcp_server::chunk_received, this, std::placeholders::_1, std::placeholders::_2, socket);
      socket->async_read(receiver);

      conns_.emplace(socket, nullptr);
    }

  private:
    void chunk_received(const char* data, int len, std::shared_ptr<tcp_socket>& socket) {
      ssize_t acc = 0;
      bool is_finished = false;

      if (len == 0) {
        conns_.erase(socket);
        return;
      }

      socket->async_write("hello world!", [this, &socket]() {
        loop_.async_timeout(.1, [this, &socket]() {
          conns_.erase(socket);
        });
      });

    }

  private:
    using socket_t = std::shared_ptr<tcp_socket>;

    socket_t listener_;
    InputOutputHandler& loop_;
    std::unordered_map<socket_t, void*> conns_;
  };

}
}
