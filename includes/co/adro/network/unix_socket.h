#pragma once
#include <string>
#include <co/adro/network/async_socket.h>

namespace co{
namespace adro{
namespace event_loop{
  class EventLoopEV;
}
namespace network{

  class UnixSocket : public AsyncSocket
  {
  public:
    UnixSocket(event_loop::EventLoopEV &io);
    UnixSocket(event_loop::EventLoopEV &io, int fd);
    int Connect(const std::string& path);
    bool Bind(const std::string& path);

  };

}
}
}
