#pragma once

#include "async_socket.hpp"

namespace async_redis {
  namespace network
  {
    class tcp_socket : public async_socket
    {
    public:
      tcp_socket(event_loop::event_loop_ev& io);

      void async_connect(const string& ip, int port, connect_handler_t handler);
      bool bind(const string& host, int port);
      int connect(const string& host, int port);
    };
  }
}
