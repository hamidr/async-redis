#pragma once

#include <stdlib.h>
#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <iostream>
#include <mutex>
#include <thread>
#include <array>
#include <list>
#include <numeric>
#include <sstream>
#include <unordered_map>

#include <ev.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/fcntl.h> // fcntl
#include <unistd.h> // close
#include <netinet/in.h>
#include <arpa/inet.h>

// #define LOG_THIS printf("%s\n", __PRETTY_FUNCTION__);
#define LOG_THIS ;

#define LOG_ME(x) std::cout << x << std::endl;
// #define LOG_ME(x) ;


namespace async_redis
{
  namespace tcp_server
  {
  class test_parser
  {
  public:
    using parser = std::shared_ptr<std::pair<int, string>>;

    test_parser(parser &ptr)
    {
    }

    int append_chunk(const char* chunk, ssize_t length, bool &is_finished) {
      LOG_THIS;

      is_finished = true;
      return length;
    }
  };


  template<typename InputOutputHandler, typename ParserPolicy>
  class tcp_server
  {
  public:
    using parser_t        = typename ParserPolicy::parser;
    using receive_cb_t      = std::function<void (parser_t)>;

    tcp_server(InputOutputHandler &event_loop)
      : loop_(event_loop) {
      listener_ = std::make_shared<tcp_socket>();
    }

    void listen(int port) {
      if (!listener_->bind("127.0.0.1", port) || !listener_->listen())
        throw;
      LOG_THIS;

      auto receiver = std::bind(&tcp_server::accept, this, std::placeholders::_1);
      listener_watcher_ = listener_->async_accept<InputOutputHandler, tcp_socket>(loop_, receiver);
    }

    void accept(std::shared_ptr<tcp_socket> socket) {
      LOG_THIS;

      auto receiver = std::bind(&tcp_server::chunk_received, this, std::placeholders::_1, std::placeholders::_2, socket);
      auto&& watcher = socket->async_read(loop_, receiver);

      conns_.emplace(socket, std::make_tuple(nullptr, std::move(watcher)));
    }

    void data_received(parser_t& data) {
      LOG_ME(data->second);
    }

  private:
    void chunk_received(const char* data, ssize_t len, std::shared_ptr<tcp_socket>& socket) {
      LOG_THIS;
      ssize_t acc = 0;
      bool is_finished = false;

      if (len == 0) {
        conns_.erase(socket);
        return;
      }

      write_watcher_ = socket->async_write(loop_, "hello world!", [this, &socket]() {
          event_watcher_t w;
          w.swap(write_watcher_);

          loop_.async_timeout(1, [this, &socket]() {
              conns_.erase(socket);
            });
        });

    }

  private:
    using event_watcher_t = typename InputOutputHandler::event_watcher_t;
    using socket_t = std::shared_ptr<tcp_socket>;

    socket_t listener_;
    InputOutputHandler& loop_;
    event_watcher_t listener_watcher_;
    event_watcher_t write_watcher_;
    std::unordered_map<socket_t, std::tuple<parser_t, event_watcher_t>> conns_;
  };

  }
}
