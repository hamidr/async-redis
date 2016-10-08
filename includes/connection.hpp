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
      using event_watcher_t = std::unique_ptr<typename InputOutputHandler::event_watcher>;

    public:
      using parser_t        = typename ParserPolicy::parser;
      using reply_cb_t      = std::function<void (parser_t)>;

      connection(InputOutputHandler &event_loop)
        : event_loop_(event_loop) {
        socket_ = std::make_shared<SocketType>();
      }

      template<typename ...Args>
      void connect(Args... args) {
        event_loop_.connect(*socket_, args...);
      }

      ~connection() {
        event_loop_.disconnect(*socket_);
      }

      bool is_connected() const
      { return socket_ && socket_->is_connected(); }

      inline int pressure() const {
        return req_queue_.size();
      }

      void send(const string& command, const reply_cb_t& reply_cb) {
        // LOG_THIS
        auto receiver = std::bind(&connection::reply_received, this, std::placeholders::_1, std::placeholders::_2);

        auto &&watcher = socket_->async_write_then_read(event_loop_, command, receiver);

        req_queue_.emplace(std::move(watcher), reply_cb, nullptr);
      }

    private:
      void reply_received(const char* data, ssize_t len) {
        // LOG_THIS
        ssize_t acc = 0;
        while (acc < len) {
          auto& request = req_queue_.front();

          //TODO: Delete this line
          auto &result = std::get<0>(request);

          auto &cb = std::get<1>(request);
          auto &parser = std::get<2>(request);

          if (0 != len && -1 != len) {

            bool is_finished = false;
            acc += ParserPolicy(parser).append_chunk(data + acc, len - acc, is_finished);

            if (!is_finished)
              return;

            try {
              cb(parser);
            } catch (std::exception &e) {
              //Log stuffs!
            }

            req_queue_.pop(); //free the resources
          }
        }
      }

    private:
      std::shared_ptr<SocketType> socket_;
      InputOutputHandler& event_loop_;
      std::queue<std::tuple<event_watcher_t, reply_cb_t, parser_t>> req_queue_;
    };

  }
}
