#pragma once

#include <vector>
#include <string>
#include <memory>

#include "connection.hpp"
#include "network/async_socket.hpp"
#include "parser/redis_response.h"

namespace async_redis {
  namespace redis_impl
  {
    using std::string;

    template<typename InputOutputHandler, typename SocketType>
    class redis_client
    {
      using connection_t = connection<InputOutputHandler, SocketType, ::async_redis::parser::redis_response>;
      using reply_cb_t = typename connection_t::reply_cb_t;
      using connect_handler_t = typename async_redis::network::async_socket<SocketType>::connect_handler_t;

    public:
      using parser_t = typename connection_t::parser_t;

      redis_client(InputOutputHandler &eventIO, int n = 1)
        : ev_loop_(eventIO)
      {
        conn_pool_.reserve(n);

        for (int i = 0; i < conn_pool_.capacity(); ++i)
          conn_pool_.push_back(std::make_unique<connection_t>(ev_loop_));
      }

      template <typename ...Args>
      void connect(const connect_handler_t& handler, Args... args) {
        for(auto &conn : conn_pool_)
          conn->connect([&](bool res) {

              ++connected_called_;
              if (connected_called_ != conn_pool_.size())
                return;

              bool val = true;
              for(auto &con : conn_pool_)
                val &= con->is_connected();

              return handler(val);
            }, args...);
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

      void select(uint catalog, reply_cb_t reply) {
        send({"select", std::to_string(catalog)}, reply);
      }

      //just to cause error!
      void err(reply_cb_t reply) {
        send({"set 1"}, reply);
      }


      // all the commands
      // with awesome good interfaces for libraries

    private:
      void send(const std::vector<string>& elems, reply_cb_t reply) {
        auto &conn = get_connection();
        string cmd;
        for (auto &s : elems)
          cmd += s + " ";

        cmd += "\r\n";
        conn.send(cmd, reply);
      }

      connection_t& get_connection() {
        // LOG_THIS

        auto &con = conn_pool_[round_robin_ctr_++];
        if (round_robin_ctr_ == conn_pool_.size())
          round_robin_ctr_ = 0;

        return *con.get();
      }

    private:
      std::vector<std::unique_ptr<connection_t>> conn_pool_;
      InputOutputHandler& ev_loop_;
      int round_robin_ctr_ = 0;
      int connected_called_ = 0;
    };
  }
}
