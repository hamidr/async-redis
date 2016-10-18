#pragma once

#include <vector>
#include <string>
#include <memory>

#include <co/adro/redis/connection.h>
#include <co/adro/network/async_socket.h>
#include <co/adro/redis/parser/redis_response.h>

namespace co{
namespace adro{
namespace event_loop{
  class EvenetLoopEV;
}
namespace redis{
    class RedisClient
    {
    public:
      RedisClient(event_loop::EventLoopEV &eventIO,network::AsyncSocket* socket,int n = 1);
      template <typename ...Args>
      void Connect(const std::function<void(bool)>& handler, Args... args);
      void Set(const std::string& key, const std::string& value, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply);
      void Get(const std::string& key, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply);
      void Keys(const std::string& pattern,  std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply);
      void HGetAll(const std::string& field, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply);
      void HMGet(const std::string& hash_name, std::vector<std::string> fields, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply);
      void Incr(const std::string& field,  std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply);
      void Decr(const std::string& field,  std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply);
      //just to cause error!
      void Err( std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply);
      // all the commands
      // with awesome good interfaces for libraries

    private:
      void Send(const std::vector<std::string>& elems, std::function<void(std::shared_ptr<parser::base_resp_parser> )> reply);
      Connection& GetConnection();
    private:
      std::vector<std::unique_ptr<Connection>> connPool_;
      event_loop::EventLoopEV& evLoop_;
      int roundRobinCtr_;
      int connectedCalled_;
    };
}
}
}
