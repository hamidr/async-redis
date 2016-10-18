#pragma once

#include <queue>
#include <functional>
#include <memory>
#include <tuple>
#include <co/adro/redis/parser/redis_response.h>
#include <co/adro/network/async_socket.h>
#include <co/adro/event_loop/event_loop_ev.h>

namespace co{
namespace adro{
namespace event_loop{
  class EventLoopEV;
}
namespace redis{
    class Connection
    {
    public:
      Connection(event_loop::EventLoopEV &eventLoop, network::AsyncSocket* socket);
      template<typename ...Args>
      inline void Connect(Args... args) ;
      bool IsConnected() const;
      inline int Pressure() const ;
      void Send(const std::string& command, const std::function<void(std::shared_ptr<parser::base_resp_parser>)>& reply_cb) ;

    private:
      void ReplyReceived(const char* data, int len) ;

    private:
      std::shared_ptr<network::AsyncSocket> socket_;
      event_loop::EventLoopEV& eventLoop_;
      std::queue<std::tuple<std::function<void(std::shared_ptr<parser::base_resp_parser>)>, std::shared_ptr<parser::base_resp_parser> > > reqQueue_;
      // enum {max_length = 1024};
      // char buffer_[max_length] = {0};
    };

}
}
}
