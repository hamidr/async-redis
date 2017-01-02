#pragma once

#include <parser/base_resp_parser.h>
#include <parser/redis_response.h>
#include <monitor.hpp>
#include <queue>
#include <functional>
#include <connection.hpp>
#include <memory>
#include <istream>
#include <sstream>

namespace async_redis {
  namespace redis_impl
  {
    template<typename InputOutputHandler>
    class sentinel
    {
      using socket_t     = ::async_redis::network::tcp_socket<InputOutputHandler>;
      using connect_cb_t = typename socket_t::connect_handler_t;

      using redis_resp_t = ::async_redis::parser::redis_response;
      using monitor_t    = monitor<InputOutputHandler, socket_t, redis_resp_t>;
      using connection_t = connection<InputOutputHandler, socket_t, redis_resp_t>;


    public:
      using parser_t     = redis_resp_t::parser;

      enum SentinelState {
        Disconnected,
        Watching
      };

      sentinel(InputOutputHandler &event_loop)
        : conn_(std::make_unique<connection_t>(event_loop)),
          stream_(std::make_unique<monitor_t>(event_loop))
      { }

      bool is_connected() const
      { return stream_->is_connected() && conn_->is_connected(); }

      bool connect(const string& ip, int port, connect_cb_t&& connector)
      {
        if (is_connected())
          return false;

        connect_all(ip, port, connector);
        return true;
      }

      void disconnect() {
        stream_->disconnect();
        conn_->disconnect();
      }

      // void sentinels();
      // void masters();
      // void slaves();
      // void reset();
      // void flushconfig();

      inline
      bool failover(const string& clustername, typename connection_t::reply_cb_t&& reply)
      {
        return if_connected_do(
          [&]() -> bool {
            return send({std::string("sentinel failover ") + clustername}, std::move(reply));
          }
        );
      }

      inline
      bool ping(typename connection_t::reply_cb_t&& reply)
      {
        return if_connected_do(
          [&]() -> bool {
            return send({"ping"}, std::move(reply));
          }
        );
      }

      using cb_watch_master_change_t = std::function<void (const std::vector<std::string>&& info, SentinelState state)>;

      inline
      bool watch_master_change(cb_watch_master_change_t&& fn)
      {
        return if_connected_do(
          [&]()-> bool {
            using State = typename monitor_t::EventState;

            return stream_->subscribe({"+switch-master"},
              [this, fn = std::move(fn)](const string& channel, parser_t event, State state) -> void
              {
                switch(state)
                {
                case State::Stream:
                  return fn(parse_watch_master_change(event), SentinelState::Watching);

                case State::Disconnected:
                  this->disconnect();
                  return fn({}, SentinelState::Disconnected);
                  break;

                default:
                  break;
                }
              }
            );
          }
        );
      }


      using cb_addr_by_name_t = std::function<void(const string&, int, bool res)>;

      inline
      bool master_addr_by_name(const string& cluster_name, cb_addr_by_name_t&& cb)
      {
        return if_connected_do(
          [&]() -> bool {
            return send_master_addr_by_name(cluster_name, std::move(cb));
          }
        );
      }

    private:
      bool send_master_addr_by_name(const string& cluster_name, cb_addr_by_name_t&& cb)
      {
        return send(string("SENTINEL get-master-addr-by-name ") + cluster_name + "\r\n",
          [cb = std::move(cb)](parser_t parsed_value)
          {
            using ::async_redis::parser::RespType;

            if (parsed_value->type() == RespType::Err)
              return cb(nullptr, -1, false);

            int elems = 0;

            string addr;
            int port = -1;

            parsed_value->map(
              [&elems, &addr, &port](const parser::base_resp_parser& val)
              {
                switch(elems)
                {
                case 0:
                  addr = val.to_string();
                  break;
                case 1:
                  port = std::stoi(val.to_string());
                  break;
                }

                ++elems;
              }
            );

            cb(addr, port, elems != 2);
          }
        );
      }

      static
      std::vector<std::string> parse_watch_master_change(const parser_t& event)
      {
        std::vector<string> words;
        std::istringstream iss(event->to_string());

        std::string s;
        while ( getline( iss, s, ' ' ) )
          words.push_back(s);

        return words;
      }

    private:
      inline
      bool if_connected_do(std::function<bool ()>&& fn)
      {
        if (!is_connected())
          return false;

        return fn();
      }

      void connect_all(const string& ip, int port, const connect_cb_t& connector)
      {
        auto cb = std::bind(&sentinel::check_connected, this, connector, std::placeholders::_1);

        conn_->connect(cb, ip, port);
        stream_->connect(cb, ip, port);
      }

      void check_connected(const connect_cb_t& connector, bool res)
      {
        if (++connected_ != 2)
          return;

        if (!is_connected())
          disconnect();

        connected_ = 0;
        connector(is_connected());
      }

      bool send(std::list<string>&& words, typename connection_t::reply_cb_t&& reply)
      {
        string cmd = words.front();
        words.pop_front();
        for(auto &w : words)
          cmd += " " + w;
        cmd += "\r\n";

        if (!conn_->send(std::move(cmd), std::move(reply))) {
          disconnect();
          return false;
        }

        return true;
      }

    private:
      int connected_ = 0;
      std::unique_ptr<monitor_t> stream_;
      std::unique_ptr<connection_t> conn_;
    };

  }
}
