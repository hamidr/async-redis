#pragma once

#include "async_socket.hpp"

namespace async_redis {
  namespace network
  {
    class tcp_socket : public async_socket
    {

    public:
      tcp_socket(event_loop::event_loop_ev& io)
        : async_socket(io)
      {
        this->create_socket(AF_INET);
      }

      tcp_socket(event_loop::event_loop_ev& io, int fd)
        : async_socket(io, fd)
      {}

      bool bind(const string& host, int port)
      {
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port   = ::htons(port);
        addr.sin_addr.s_addr = inet_addr(host.data());


        return this->bind_to((socket_t *)&addr, sizeof(addr)) == 0;
      }

      int connect(const string& host, int port)
      {
        //TODO:
        // setsockopt (fd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on));
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port   = ::htons(port);
        addr.sin_addr.s_addr = inet_addr(host.data());

        return this->connect_to((socket_t *)&addr, sizeof(addr));
      }
    };
  }
}
