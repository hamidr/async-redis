#pragma once

#include <queue>
#include <functional>
#include <memory>
#include <tuple>

namespace async_redis {
  namespace redis_impl
  {
    using std::string;

    template<typename InputOutputHandler, typename SocketType, typename ParserPolicy>
    class connection
    {
    public:
      using parser_t        = typename ParserPolicy::parser;
      using reply_cb_t      = std::function<void (parser_t)>;

      connection(InputOutputHandler &event_loop)
        : event_loop_(event_loop) {
        socket_ = std::make_unique<SocketType>(event_loop);
      }

      template<typename ...Args>
      inline void connect(Args... args) {
        socket_->template async_connect<SocketType>(0, std::forward<Args>(args)...);
      }

      bool is_connected() const
      { return socket_ && socket_->is_connected(); }

      inline int pressure() const {
        return req_queue_.size();
      }

      void send(const string& command, const reply_cb_t& reply_cb) {

        socket_->async_write(command, [this, reply_cb]() {
            req_queue_.emplace(reply_cb, nullptr);

            if (req_queue_.size() == 1)
              socket_->async_read(std::bind(&connection::reply_received, this, std::placeholders::_1, std::placeholders::_2));
          });
      }

    private:
      void reply_received(const char* data, int len) {
        ssize_t acc = 0;

        while (acc < len && req_queue_.size()) {
          auto& request = req_queue_.front();

          auto &cb = std::get<0>(request);
          auto &parser = std::get<1>(request);

          if (0 != len && -1 != len) {

            bool is_finished = false;
            acc += ParserPolicy(parser).append_chunk(data + acc, len - acc, is_finished);

            if (!is_finished)
              break;

            cb(parser);
            req_queue_.pop(); //free the resources
          }
        }

        if (req_queue_.size() != 0)
          socket_->async_read(std::bind(&connection::reply_received, this, std::placeholders::_1, std::placeholders::_2));
      }

    private:
      std::unique_ptr<SocketType> socket_;
      InputOutputHandler& event_loop_;
      std::queue<std::tuple<reply_cb_t, parser_t>> req_queue_;
    };

  }
}
