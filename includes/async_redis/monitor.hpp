#pragma once

#include <libevpp/network/async_socket.hpp>
#include <parser/base_resp_parser.h>

#include <unordered_map>
#include <list>
#include <string>

using std::string;
using namespace libevpp;

namespace async_redis
{
  class monitor
  {
    using async_socket    = network::async_socket;

  public:
    enum EventState {
      Sub,
      Unsub,
      Stream,
      Disconnected
    };

    using parser_t     = parser::base_resp_parser::parser;
    using watcher_cb_t = std::function<void (const string&, parser_t, EventState)>;

    monitor(event_loop::event_loop_ev &event_loop);

    void connect(async_socket::connect_handler_t handler, const std::string& ip, int port);
    void connect(async_socket::connect_handler_t handler, const std::string& path);

    bool is_connected() const;
    bool is_watching() const;
    void disconnect();

    bool psubscribe(const std::list<string>& channels, watcher_cb_t&& cb);
    bool subscribe(const std::list<string>& channels, watcher_cb_t&& cb);
    bool unsubscribe(const std::list<string>& channels, watcher_cb_t&& cb);
    bool punsubscribe(const std::list<string>& channels, watcher_cb_t&& cb);

  private:
    bool send_and_receive(string&& data);
    void handle_message_event(parser_t& channel, parser_t& value);
    void handle_subscribe_event(parser_t& channel, parser_t& clients);
    void handle_psubscribe_event(parser_t& channel, parser_t& clients);
    void handle_punsubscribe_event(parser_t& pattern, parser_t& clients);
    void handle_unsubscribe_event(parser_t& channel, parser_t& clients);
    void handle_pmessage_event(parser_t& pattern, parser_t& channel, parser_t& value);
    void handle_event(parser_t&& request);
    void report_disconnect();

    void stream_received(ssize_t len);

  private:
    parser_t parser_;
    std::unordered_map<std::string, watcher_cb_t> watchers_;
    std::unordered_map<std::string, watcher_cb_t> pwatchers_;

    std::unique_ptr<async_socket> socket_;
    event_loop::event_loop_ev &io_;
    enum { max_data_size = 1024 };
    char data_[max_data_size];
    bool is_watching_ = false;
  };

}
