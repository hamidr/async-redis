#pragma once

#include <connection.hpp>
#include <list>

namespace async_redis {
  namespace redis_impl
  {
    using std::string;

    template<typename InputOutputHandler, typename SocketType, typename ParserPolicy>
    class monitor : protected connection<InputOutputHandler, SocketType, ParserPolicy>
    {
      using connection_t = connection<InputOutputHandler, SocketType, ParserPolicy>;
      using reply_cb_t   = typename connection_t::reply_cb_t;

    public:
      enum State {
        StartResult,
        StopResult,
        EventStream
      };

      using parser_t     = typename ParserPolicy::parser;
      using watcher_cb_t = std::function<bool (parser_t, State)>;

      monitor(InputOutputHandler &event_loop)
        : connection_t(event_loop)
      { }

      template<typename ...Args>
      inline void connect(Args... args) {
        connection_t::connect(std::forward<Args>(args)...);
      }

      inline bool is_connected() const
      { return connection_t::is_connected(); }

      bool is_watching() const
      { return this->is_connected() && watcher_; }

      void disconnect()
      {
        watcher_ = nullptr;
        connection_t::disconnect();
      }

      //TODO: Handle monitor later!
      // bool monitoring(watcher_cb_t&& reply_cb)

      bool psubscribe(const std::list<string>& channels, watcher_cb_t&& cb)
      {
        return watch("psubscribe", "punsubscribe", channels, std::move(cb));
      }

      bool subscribe(const std::list<string>& channels, watcher_cb_t&& cb)
      {
        return watch("subscribe", "unsubscribe", channels, std::move(cb));
      }

    private:
      bool set_watcher_cb(watcher_cb_t&& watch_cb)
      {
        if (watcher_)
          return false;

        watcher_ = std::make_unique<watcher_cb_t>(std::move(watch_cb));

        return true;
      }

      inline bool call_watcher(parser_t parser, State state)
      {
        return watcher_ ? (*watcher_)(parser, state) : false;
      }

      bool watch(string&& start_cmd, string&& stop_cmd, const std::list<string>& channels, watcher_cb_t &&watch_cb)
      {
        if (!channels.size())
          return false;

        for(auto &ch : channels) {
          start_cmd += " " + ch;
          stop_cmd += " " + ch;
        }

        start_cmd += "\r\n";
        stop_cmd += "\r\n";

        if (!set_watcher_cb(std::move(watch_cb)))
          return false;

        this->send(std::move(start_cmd),
          [this, stop_cmd = std::move(stop_cmd)](parser_t resp)
          {
            if (!call_watcher(std::move(resp), State::StartResult))
              return;

            this->socket_->async_read(
              this->data_,
              this->max_data_size,
              std::bind(
                &monitor::stream_received,
                this,
                std::move(stop_cmd),
                std::placeholders::_1
              )
            );
          }
        );

        return true;
      }

      void stream_received(const string& stop_cmd, ssize_t len)
      {
        if (len < 1)
          return;

        ssize_t acc = 0;
        while (acc < len && watcher_)
        {
          bool is_finished = false;
          acc += ParserPolicy(parser_).append_chunk(this->data_ + acc, len - acc, is_finished);

          if (!is_finished)
            break;

          bool res = false;
          { // pass the parser and be done with it
            parser_t event;
            std::swap(event, parser_);

            res = call_watcher(std::move(event), State::EventStream);
          }

          if (!res) {
            this->send(std::move(stop_cmd), std::bind(&monitor::call_watcher, this, std::placeholders::_1, State::StopResult));
            return;
          }
        }

        this->socket_->async_read(
          this->data_,
          this->max_data_size,
          std::bind(
            &monitor::stream_received,
            this,
            std::move(stop_cmd),
            std::placeholders::_1
          )
        );
      }

    private:
      std::unique_ptr<watcher_cb_t> watcher_;
      parser_t parser_;
    };

  }
}
