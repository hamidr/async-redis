#pragma once

#include <parser/base_resp_parser.h>

#include <unordered_map>
#include <list>
#include <string>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

using std::string;

namespace async_redis
{
  class monitor
  {
    monitor(const monitor&) = delete;
    monitor& operator = (const monitor&) = delete;

    using connect_handler_t = std::function<void(bool)>;

  public:
    enum EventState {
      Sub,
      Unsub,
      Stream,
      Disconnected
    };

    using parser_t     = parser::base_resp_parser::parser;
    using watcher_cb_t = std::function<void (const string&, parser_t, EventState)>;

    monitor(asio::io_context &event_loop);
    void connect(connect_handler_t handler, const std::string& ip, int port);

    bool is_connected() const;
    bool is_watching() const;
    void disconnect();

    bool psubscribe(const std::list<string>& channels, watcher_cb_t&& cb);
    bool subscribe(const std::list<string>& channels, watcher_cb_t&& cb);
    bool unsubscribe(const std::list<string>& channels, watcher_cb_t&& cb);
    bool punsubscribe(const std::list<string>& channels, watcher_cb_t&& cb);

  private:
    void do_read();
    bool send_and_receive(string&& data);
    void handle_message_event(parser_t& channel, parser_t& value);
    void handle_subscribe_event(parser_t& channel, parser_t& clients);
    void handle_psubscribe_event(parser_t& channel, parser_t& clients);
    void handle_punsubscribe_event(parser_t& pattern, parser_t& clients);
    void handle_unsubscribe_event(parser_t& channel, parser_t& clients);
    void handle_pmessage_event(parser_t& pattern, parser_t& channel, parser_t& value);
    void handle_event(parser_t&& request);
    void report_disconnect();

    void stream_received(const asio::error_code& ec, size_t len);

  private:
    parser_t parser_;
    std::unordered_map<std::string, watcher_cb_t> watchers_;
    std::unordered_map<std::string, watcher_cb_t> pwatchers_;

    asio::ip::tcp::socket socket_;
    enum { max_data_size = 1024 };
    char data_[max_data_size];
    bool is_watching_ = false;
  };

}
