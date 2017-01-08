#pragma once
#include <parser/base_resp_parser.h>

#include <functional>

#include <monitor.hpp>
#include <connection.hpp>

namespace async_redis {
  class sentinel
  {
    using connect_cb_t = connection::connect_handler_t;

  public:
    using parser_t     = parser::base_resp_parser::parser;

    enum SentinelState {
      Disconnected,
      Watching
    };

    sentinel(asio::io_context &io);

    bool is_connected() const;
    bool connect(const string& ip, int port, connect_cb_t&& connector);
    void disconnect();

    // void sentinels();
    // void masters();
    // void slaves();
    // void reset();
    // void flushconfig();

    bool failover(const string& clustername, connection::reply_cb_t&& reply);
    bool ping(connection::reply_cb_t&& reply);

    using cb_watch_master_change_t = std::function<void (const std::vector<std::string>&& info, SentinelState state)>;
    bool watch_master_change(cb_watch_master_change_t&& fn);

    using cb_addr_by_name_t = std::function<void(const string&, int, bool res)>;
    bool master_addr_by_name(const string& cluster_name, cb_addr_by_name_t&& cb);

  private:
    bool send_master_addr_by_name(const string& cluster_name, cb_addr_by_name_t&& cb);
    static std::vector<std::string> parse_watch_master_change(const parser_t& event);

  private:
    bool if_connected_do(std::function<bool ()>&& fn);
    void connect_all(const string& ip, int port, const connect_cb_t& connector);
    void check_connected(const connect_cb_t& connector, bool res);
    bool send(std::list<string>&& words, connection::reply_cb_t&& reply);

  private:
    int connected_ = 0;
    monitor stream_;
    connection conn_;
  };
}
