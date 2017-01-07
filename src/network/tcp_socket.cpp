#include "../../includes/network/tcp_socket.hpp"

#include <arpa/inet.h>

namespace async_redis {
namespace network {

tcp_socket::tcp_socket(event_loop::event_loop_ev& io)
  : async_socket(io)
{
  this->create_socket(AF_INET);
}

void tcp_socket::async_connect(const string& ip, int port, connect_handler_t handler)
{
  async_socket::template async_connect<tcp_socket>(0, handler, ip, port);
}

bool tcp_socket::bind(const string& host, int port)
{
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(port);
  addr.sin_addr.s_addr = inet_addr(host.data());

  return this->bind_to((socket_t *)&addr, sizeof(addr)) == 0;
}

int tcp_socket::connect(const string& host, int port)
{
  //TODO:
  // setsockopt (fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));

  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_port   = htons(port);
  addr.sin_addr.s_addr = inet_addr(host.data());

  return this->connect_to((socket_t *)&addr, sizeof(addr));
}

}
}
