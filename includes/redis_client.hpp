#pragma once

#include <vector>
#include <string>
#include <memory>

#include "connection.hpp"
#include "network/async_socket.hpp"
#include "parser/redis_response.h"

namespace async_redis
{
  using std::string;

  template<typename InputOutputHandler>
  class redis_client
  {
    using reply_cb_t   = connection::reply_cb_t;
    using connect_cb_t = network::async_socket::connect_handler_t;

  public:
    class connect_exception : std::exception {};
    using parser_t = connection::parser_t;

    redis_client(InputOutputHandler &eventIO, int n = 1)
      : ev_loop_(eventIO)
    {
      conn_pool_.reserve(n);

      for (int i = 0; i < conn_pool_.capacity(); ++i)
        conn_pool_.push_back(std::make_unique<connection>(ev_loop_));
    }

    bool is_connected() const
    { return is_connected_; }

    template <typename ...Args>
    void connect(const connect_cb_t& handler, Args... args) {
      for(auto &conn : conn_pool_)
        conn->connect(std::bind(&redis_client::check_conn_connected, this, handler, std::placeholders::_1), args...);
    }

    void disconnect() {
      for(auto &conn : conn_pool_)
        conn->disconnect();
    }

    void set(const string& key, const string& value, reply_cb_t reply) {
      send({"set", key, value}, reply);
    }

    void get(const string& key, reply_cb_t reply) {
      send({"get", key}, reply);
    }

    void keys(const string& pattern, reply_cb_t reply) {
      send({"keys", pattern}, reply);
    }

    void hgetall(const string& field, reply_cb_t reply) {
      send({"hgetall", field}, reply);
    }

    void hmget(const string& hash_name, std::vector<string> fields, reply_cb_t reply) {
      string req;
      for (auto &field : fields)
        req += field + " ";

      send({"hmget", hash_name, req}, reply);
    }

    void incr(const string& field, reply_cb_t reply) {
      send({"incr", field}, reply);
    }

    void decr(const string& field, reply_cb_t reply) {
      send({"decr", field}, reply);
    }

    void ping(reply_cb_t reply) {
      send({"ping"}, reply);
    }

    //TODO: wtf?! doesnt make sense with multiple connections!
    // void select(uint catalog, reply_cb_t&& reply) {
    //   send({"select", std::to_string(catalog)}, reply);
    // }

    void publish(const string& channel, const string& msg, reply_cb_t&& reply) {
      send({"publish", channel, msg}, reply);
    }

    void commit_pipeline() {
      string buffer;
      std::swap(pipeline_buffer_, buffer);
      std::vector<reply_cb_t> cbs;
      pipelined_cbs_.swap(cbs);

      is_connected_ = get_connection().pipelined_send(std::move(buffer), std::move(cbs));
      if (!is_connected_) {
        disconnect();
        throw connect_exception();
      }
    }

    redis_client& pipeline_on() {
      pipelined_state_ = true;
      return *this;
    }

    redis_client& pipeline_off() {
      pipelined_state_ = false;
      return *this;
    }


  private:
    void send(const std::vector<string>&& elems, const reply_cb_t& reply)
    {
      if (!is_connected_)
        throw connect_exception();

      string cmd;
      for (auto &s : elems)
        cmd += s + " ";

      cmd += "\r\n";

      if (!pipelined_state_) {
        is_connected_ = get_connection().send(std::move(cmd), reply);
        if(!is_connected_) {
          disconnect();
          throw connect_exception();
        }
        return;
      }

      pipeline_buffer_ += cmd;
      pipelined_cbs_.push_back(std::move(reply));
    }

    connection& get_connection()
    {
      auto &con = conn_pool_[con_rr_ctr_++];
      if (con_rr_ctr_ == conn_pool_.size())
        con_rr_ctr_ = 0;

      return *con.get();
    }

    void check_conn_connected(const connect_cb_t& handler, bool res)
    {
      ++connected_called_;
      if (connected_called_ != conn_pool_.size())
        return;

      bool val = true;
      for(auto &con : conn_pool_)
        val &= con->is_connected();

      if (!val) {
        for(auto &con : conn_pool_)
          con->disconnect();
      }

      connected_called_ = 0;
      is_connected_ = val;
      return handler(val);
    }

  private:
    std::string pipeline_buffer_;
    bool pipelined_state_ = false;
    std::vector<reply_cb_t> pipelined_cbs_;
    std::vector<std::unique_ptr<connection>> conn_pool_;
    int con_rr_ctr_ = 0;
    int connected_called_ = 0;
    bool is_connected_ = false;
    InputOutputHandler& ev_loop_;
  };
}
