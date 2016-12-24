#pragma once

#include <parser/base_resp_parser.h>
#include <parser/redis_response.h>
#include <monitor.hpp>
#include <queue>
#include <functional>
#include <connection.hpp>
#include <memory>

namespace async_redis {
  namespace redis_impl
  {
    template<typename InputOutputHandler>
    class sentinel
    {
      using redis_resp_t = ::async_redis::parser::redis_response;
      using parser_t     = redis_resp_t::parser;
      using socket_t     = ::async_redis::network::tcp_socket<InputOutputHandler>;
      using connect_cb_t = typename socket_t::connect_handler_t;

      using monitor_t    = monitor<InputOutputHandler, socket_t, redis_resp_t>;
      using connection_t = connection<InputOutputHandler, socket_t, redis_resp_t>;

    public:
      sentinel(InputOutputHandler &event_loop)
        : conn_(std::make_unique<connection_t>(event_loop)),
          stream_(std::make_unique<monitor_t>(event_loop))
      { }

      bool is_connected() const
      { return is_connected_; }

      bool connect(const string& ip, int port, connect_cb_t&& connector)
      {
        if (is_connected())
          return false;

        connect_all(ip, port, connector);
        return true;
      }

      // void failover();
      // void sentinels();
      // void masters();
      // void slaves();
      // void reset();
      // void flushconfig();

      inline
      bool ping(typename connection_t::reply_cb_t&& reply)
      {
        return if_connected_do(
          [&]() -> bool {
            conn_->send({"ping"}, std::move(reply));
            return true; //TODO: return with send! fix send.
          }
        );
      }

      using cb_watch_master_change_t = std::function<bool (const string&, int)>;

      inline
      bool watch_master_change(cb_watch_master_change_t&& fn)
      {
        return if_connected_do(
          [&]()-> bool {
            using State = typename monitor_t::State;

            return stream_->subscribe({"+switch-master"},
              [fn = std::move(fn)](parser_t event, State state) -> bool
              {
                switch(state)
                {
                  case State::StartResult:
                    return true;

                  case State::StopResult:
                    return false;

                  case State::EventStream:
                    auto &&res = parse_watch_master_change(event);
                    return fn(std::get<0>(res), std::get<1>(res));
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
            send_master_addr_by_name(cluster_name, std::move(cb));
            return true;
          }
        );
      }

    private:
      void send_master_addr_by_name(const string& cluster_name, cb_addr_by_name_t&& cb)
      {
        conn_->send(string("SENTINEL get-master-addr-by-name ") + cluster_name + "\r\n",
          [cb = std::move(cb)](parser_t parsed_value)
          {
            using ::async_redis::parser::RespType;

            if (parsed_value->type() == RespType::Err)
              cb(nullptr, -1, false);

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

      static std::tuple<std::string, int> parse_watch_master_change(const parser_t& event)
      {
        string ip;
        int port = -1;

        int size = 0;
        event->map(
          [&](const parser::base_resp_parser& elem) {

            switch(size)
            {
            case 0:
              ip = elem.to_string();
              break;

            case 1:
              port = std::stoi(elem.to_string());
              break;

            default:
              std::cout << "wtffFFF" << std::endl;
            }

            ++size;
          }
        );

        return std::make_tuple(ip, port);
      }

    private:
      inline
      bool if_connected_do(std::function<bool ()>&& fn)
      {
        if (is_connected())
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

        is_connected_ = conn_->is_connected() && stream_->is_connected();

        if (!is_connected_) {
          conn_->disconnect();
          stream_->disconnect();
        }

        connected_ = 0;
        connector(is_connected_);
      }

    private:
      int connected_ = 0;
      bool is_connected_ = false;
      std::unique_ptr<monitor_t> stream_;
      std::unique_ptr<connection_t> conn_;
    };

  }
}
