#pragma once

#include "async_socket.hpp"

namespace async_redis {
  namespace network
  {
    class unix_socket : public async_socket
    {
    public:
      unix_socket(event_loop::event_loop_ev &io);
      void async_connect(const string& path, connect_handler_t handler);
      int connect(const string& path);
      bool bind(const string& path);
    };
  }
}
