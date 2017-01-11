#include "../includes/async_redis/redis_client.hpp"

#include <vector>
#include <string>
#include <memory>

namespace async_redis
{

redis_client::redis_client(event_loop::event_loop_ev &eventIO, int n)
{
  conn_pool_.reserve(n);

  for (int i = 0; i < conn_pool_.capacity(); ++i)
    conn_pool_.push_back(std::make_unique<connection>(eventIO));
}

bool redis_client::is_connected() const
{ return is_connected_; }


void redis_client::disconnect() {
  for(auto &conn : conn_pool_)
    conn->disconnect();
}

void redis_client::set(const string& key, const string& value, reply_cb_t reply) {
  send({"set", key, value}, reply);
}

void redis_client::get(const string& key, reply_cb_t reply) {
  send({"get", key}, reply);
}

void redis_client::keys(const string& pattern, reply_cb_t reply) {
  send({"keys", pattern}, reply);
}

void redis_client::hgetall(const string& field, reply_cb_t reply) {
  send({"hgetall", field}, reply);
}

void redis_client::hmget(const string& hash_name, std::vector<string> fields, reply_cb_t reply) {
  string req;
  for (auto &field : fields)
    req += field + " ";

  send({"hmget", hash_name, req}, reply);
}

void redis_client::incr(const string& field, reply_cb_t reply) {
  send({"incr", field}, reply);
}

void redis_client::decr(const string& field, reply_cb_t reply) {
  send({"decr", field}, reply);
}

void redis_client::ping(reply_cb_t reply) {
  send({"ping"}, reply);
}

//TODO: wtf?! doesnt make sense with multiple connections!
// void select(uint catalog, reply_cb_t&& reply) {
//   send({"select", std::to_string(catalog)}, reply);
// }

void redis_client::publish(const string& channel, const string& msg, reply_cb_t&& reply) {
  send({"publish", channel, msg}, reply);
}

void 
redis_client::sort(const string& hash_name, std::vector<string>&& fields, reply_cb_t&& reply)
{
  std::string req;
  for (auto &field : fields)
    req += "get " + field + " ";

  send({"sort " + hash_name + " by nosort",  req}, reply);
}


void redis_client::commit_pipeline() {
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

redis_client& redis_client::pipeline_on() {
  pipelined_state_ = true;
  return *this;
}

redis_client& redis_client::pipeline_off() {
  pipelined_state_ = false;
  return *this;
}

void redis_client::send(const std::vector<string>&& elems, const reply_cb_t& reply)
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

connection& redis_client::get_connection()
{
  auto &con = conn_pool_[con_rr_ctr_++];
  if (con_rr_ctr_ == conn_pool_.size())
    con_rr_ctr_ = 0;

  return *con.get();
}

void redis_client::check_conn_connected(const connect_cb_t& handler, bool res)
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

}
