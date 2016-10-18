#include <co/adro/network/tcp_socket.h>
#include <co/adro/event_loop/event_loop_ev.h>

namespace co{
namespace adro{
namespace network{
TCPSocket::TCPSocket(event_loop::EventLoopEV& io)
    : AsyncSocket(io)
{
  this->CreateSocket(AF_INET);
}

TCPSocket::TCPSocket(event_loop::EventLoopEV& io, int fd)
  : AsyncSocket(io, fd)
{}

bool 
TCPSocket::Bind(const std::string& host, int port)
{
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port   = ::htons(port);
  addr.sin_addr.s_addr = inet_addr(host.data());
  return this->BindTo((struct sockaddr *)&addr, sizeof(addr)) == 0;
}

int 
TCPSocket::Connect(const std::string& host, int port)
{
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port   = ::htons(port);
  addr.sin_addr.s_addr = inet_addr(host.data());

  return this->ConnectTo((struct sockaddr*)&addr, sizeof(addr));
}



}
}
}
