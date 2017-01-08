#pragma once

#include <vector>
#include <string>
#include <memory>

#include "connection.hpp"

namespace async_redis
{
  using std::string;

  class redis_client
  {
    redis_client(const redis_client&) = delete;
    redis_client& operator = (const redis_client&) = delete;

    using reply_cb_t   = connection::reply_cb_t;
    using connect_cb_t = connection::connect_handler_t;

  public:
    class connect_exception : std::exception {};
    using parser_t = connection::parser_t;

    redis_client(asio::io_context &io, uint n = 1);
    bool is_connected() const;

    template <typename ...Args>
    void connect(const connect_cb_t& handler, Args... args) {
      for(auto &conn : conn_pool_)
        conn->connect(std::bind(&redis_client::check_conn_connected, this, handler, std::placeholders::_1), args...);
    }

    void disconnect();
    void set(const string& key, const string& value, reply_cb_t reply);
    void get(const string& key, reply_cb_t reply);
    void keys(const string& pattern, reply_cb_t reply);
    void hgetall(const string& field, reply_cb_t reply);
    void hmget(const string& hash_name, std::vector<string> fields, reply_cb_t reply);
    void incr(const string& field, reply_cb_t reply);
    void decr(const string& field, reply_cb_t reply);
    void ping(reply_cb_t reply);
    void publish(const string& channel, const string& msg, reply_cb_t&& reply);

    //TODO: wtf?! doesnt make sense with multiple connections!
    // void select(uint catalog, reply_cb_t&& reply) {
    //   send({"select", std::to_string(catalog)}, reply);
    // }

    void commit_pipeline();
    redis_client& pipeline_on();
    redis_client& pipeline_off();

  private:
    void send(const std::vector<string>&& elems, const reply_cb_t& reply);
    connection& get_connection();
    void check_conn_connected(const connect_cb_t& handler, bool res);

  private:
    std::string pipeline_buffer_;
    bool pipelined_state_ = false;
    std::vector<reply_cb_t> pipelined_cbs_;
    std::vector<std::unique_ptr<connection>> conn_pool_;
    int con_rr_ctr_ = 0;
    int connected_called_ = 0;
    bool is_connected_ = false;
  };
}
