#include "../../includes/network/unix_socket.hpp"

#include <arpa/inet.h>
#include <unistd.h> // close
#include <sys/un.h>

namespace async_redis {
namespace network {

unix_socket::unix_socket(event_loop::event_loop_ev& io)
  : async_socket(io)
{
  this->create_socket(AF_UNIX);
}

void unix_socket::async_connect(const string& path, connect_handler_t handler)
{
  async_socket::template async_connect<unix_socket>(0, handler, path);
}

bool unix_socket::bind(const string& path)
{
  ::unlink(path.data());

  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, path.data());

  auto len = strlen(addr.sun_path) + sizeof(addr.sun_family);

  return this->bind_to((socket_t *)&addr, sizeof(addr)) == 0;
}

int unix_socket::connect(const string& path)
{
  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, path.data());
  auto len = strlen(addr.sun_path) + sizeof(addr.sun_family);

  return this->connect_to((socket_t *)&addr, sizeof(addr));
}

}
}
