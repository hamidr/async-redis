#pragma once

#include <string>
#include <memory>
#include <functional>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace async_redis {
namespace tcp_server {

  class tcp_server
  {
  public:
    using tcp_socket   = async_redis::network::tcp_socket;
    using async_socket   = async_redis::network::async_socket;

    tcp_server(event_loop::event_loop_ev &event_loop)
      : loop_(event_loop), listener_(event_loop) {
    }

    void listen(int port) {
      if (!listener_.bind("127.0.0.1", port) || !listener_.listen())
        throw;

      auto receiver = std::bind(&tcp_server::accept, this, std::placeholders::_1);
      listener_.async_accept(receiver);
    }

    void accept(std::shared_ptr<async_socket> socket) {
      auto receiver = std::bind(&tcp_server::chunk_received, this, std::placeholders::_1, socket);
      socket->async_read(buffer_, max_buffer_length, receiver);

      conns_.emplace(socket, nullptr);
    }

  private:
    void chunk_received(int len, std::shared_ptr<async_socket>& socket)
    {
      std::string command;

      if (len <= 0) {
        conns_.erase(socket);
        return;
      }

      for(int n = 0; n < len; ++n) {

        char c = buffer_[n];
        switch(c)
        {
        case '\r':
        case '\n':
          continue;
          break;

        default:
          command.push_back(c);
        }
      }

      fprintf(stdout, ("cmd: " + command + "\n").data());
      fflush(stdout);

      if (command == "close") {
        socket->async_write("good bye!", [this, &socket](ssize_t l) {
            loop_.async_timeout(1, [this, &socket]() {
                conns_.erase(socket);
              });
          });
        return; // dont read
      }

      auto receiver = std::bind(&tcp_server::chunk_received, this, std::placeholders::_1, socket);
      socket->async_read(buffer_, max_buffer_length, receiver);
    }

  private:
    using socket_t = std::shared_ptr<async_socket>;

    tcp_socket listener_;
    event_loop::event_loop_ev& loop_;
    std::unordered_map<socket_t, void*> conns_;
    enum { max_buffer_length = 1024 };
    char buffer_[max_buffer_length];
  };

}
}
