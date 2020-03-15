#pragma once

#include <vector>
#include <string>
#include <memory>

#include <async_redis/connection.hpp>

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

    redis_client(asio::io_context &io) noexcept;
    bool is_connected() const noexcept;

    template <typename ...Args>
    void connect(const connect_cb_t& handler, Args... args) {
        conn_.connect(handler, args...);
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
    void sort(const string& hash_name, std::vector<string>&& fields, reply_cb_t&& reply);
    void select(uint catalog, reply_cb_t&& reply);

  private:
    void send(std::vector<string>&& elems, const reply_cb_t& reply);

  private:
    connection conn_;
    int connected_called_ = 0;
    bool is_connected_ = false;
  };
}
