#include "../includes/async_redis/redis_client.hpp"

#include <memory>

namespace async_redis
{

redis_client::redis_client(asio::io_context &io) noexcept
  : conn_(io)
{}

bool redis_client::is_connected() const noexcept
{ return conn_.is_connected(); }

void redis_client::disconnect() {
  conn_.disconnect();
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

void redis_client::select(uint catalog, reply_cb_t&& reply) {
  send({"select", std::to_string(catalog)}, reply);
}

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

void redis_client::send(std::vector<string>&& elems, const reply_cb_t& reply)
{
  if (!is_connected())
    throw connect_exception();

  string cmd;
  for (auto &s : elems)
    cmd += s + " ";

  cmd += "\r\n";

  conn_.send(std::move(cmd), reply);
}

}
