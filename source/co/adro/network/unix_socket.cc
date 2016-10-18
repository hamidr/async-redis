#include <co/adro/network/unix_socket.h>

namespace co{
namespace adro{
namespace network{
UnixSocket::UnixSocket(event_loop::EventLoopEV &io)
  :AsyncSocket(io)
{
  this->CreateSocket(AF_UNIX);
}

UnixSocket::UnixSocket(event_loop::EventLoopEV &io, int fd)
  : AsyncSocket(io, fd)
{}

int 
UnixSocket::Connect(const std::string& path) {
  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, path.data());
  auto len = strlen(addr.sun_path) + sizeof(addr.sun_family);

  return this->ConnectTo((struct sockaddr *)&addr, sizeof(addr));
}

bool 
UnixSocket::Bind(const std::string& path) {
  ::unlink(path.data());

  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, path.data());

  auto len = strlen(addr.sun_path) + sizeof(addr.sun_family);

  return this->BindTo((struct sockaddr *)&addr, sizeof(addr)) == 0;
}


}
}
}
