#pragma once

#include <co/adro/network/async_socket.h>

namespace co{
namespace adro{
namespace event_loop{
  class EventLoopEV; 
}
namespace network{
    class TCPSocket : public AsyncSocket
    {
    public:
      TCPSocket(event_loop::EventLoopEV& io);
      TCPSocket(event_loop::EventLoopEV& io, int fd);
      bool Bind(const std::string& host, int port);
      int Connect(const std::string& host, int port);
    };
}
}
}
