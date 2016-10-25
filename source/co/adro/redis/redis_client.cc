#include <co/adro/redis/redis_client.h>
#include <co/adro/event_loop/event_loop_ev.h>

namespace co{
namespace adro{
namespace redis{
RedisClient::RedisClient(event_loop::EventLoopEV &eventIO,network::AsyncSocket* socket,int n )
  : evLoop_(eventIO)
{
  roundRobinCtr_ = 0 ;
  connectedCalled_ = 0 ;
  connPool_.reserve(n);

  for (int i = 0; i < connPool_.capacity(); ++i)
    connPool_.push_back(std::make_unique<Connection>(evLoop_, socket));
}
void 
RedisClient::Connect(const std::function<void(bool)>& handler, std::string host , int port ){
  for(auto &conn : connPool_)
    conn->Connect([&](bool res) {

        ++connectedCalled_;
        if (connectedCalled_ != connPool_.size())
          return;

        bool val = true;
        for(auto &con : connPool_)
          val &= con->IsConnected();

        return handler(val);
      }, host , port );
}


void 
RedisClient::Set(const std::string& key, const std::string& value, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  Send({"set", key, value}, reply);
}

void 
RedisClient::Get(const std::string& key, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  Send({"get", key}, reply);
}

void 
RedisClient::Keys(const std::string& pattern,  std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  Send({"keys", pattern}, reply);
}

void 
RedisClient::HGetAll(const std::string& field, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  Send({"hgetall", field}, reply);
}
void 
RedisClient::MGet( std::vector<std::string> fields, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  std::string req;
  for (auto &field : fields)
    req += field + " ";

  Send({"mget",  req}, reply);
}

void 
RedisClient::Sort( std::string hash_name, std::vector<std::string> fields, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  std::string req;
  for (auto &field : fields)
    req += "Get " + field + " ";

  Send({"sort "+hash_name+" by nosort",  req}, reply);
}




void 
RedisClient::HMGet(const std::string& hash_name, std::vector<std::string> fields, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  std::string req;
  for (auto &field : fields)
    req += field + " ";

  Send({"hmget", hash_name, req}, reply);
}

void 
RedisClient::Incr(const std::string& field,  std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  Send({"incr", field}, reply);
}

void 
RedisClient::Decr(const std::string& field,  std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  Send({"decr", field}, reply);
}

void 
RedisClient::Err( std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  Send({"set 1"}, reply);
}


void 
RedisClient::Send(const std::vector<std::string>& elems, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply) {
  auto &conn = GetConnection();
  std::string cmd;
  for (auto &s : elems)
    cmd += s + " ";

  cmd += "\r\n";
  conn.Send(cmd, reply);
}

Connection& 
RedisClient::GetConnection() {
  // LOG_THIS

  auto &con = connPool_[roundRobinCtr_++];
  if (roundRobinCtr_ == connPool_.size())
    roundRobinCtr_ = 0;

  return *con.get();
}
}
}
}
