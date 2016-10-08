#pragma once

#include "async_socket.hpp"

namespace async_redis {
  namespace network
  {
    class tcp_socket : public async_socket
    {
    public:
      tcp_socket()
      {
        create_socket(AF_INET);
      }

      tcp_socket(int fd)
        : async_socket(fd)
      {}

      bool bind(const string& host, int port)
      {
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port   = ::htons(port);
        addr.sin_addr.s_addr = inet_addr(host.data());

        return bind_to((socket_t *)&addr, sizeof(addr)) == 0;
      }

      int connect(const string& host, int port)
      {
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port   = ::htons(port);
        addr.sin_addr.s_addr = inet_addr(host.data());

        return connect_to((socket_t *)&addr, sizeof(addr));
      }
    };
  }
}
