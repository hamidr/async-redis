#pragma once

#include "async_socket.hpp"

namespace async_redis {
  namespace network
  {
    class unix_socket : public async_socket
    {
    public:
      unix_socket() {
        create_socket(AF_UNIX);
      }

      unix_socket(int fd)
        : async_socket(fd)
      {}

      int connect(const string& path) {
        struct sockaddr_un addr = {0};
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, path.data());
        auto len = strlen(addr.sun_path) + sizeof(addr.sun_family);

        return connect_to((socket_t *)&addr, len);
      }

      bool bind(const string& path) {
        struct sockaddr_un addr = {0};
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, path.data());

        auto len = strlen(addr.sun_path) + sizeof(addr.sun_family);

        return bind_to((socket_t *)&addr, len) == 0;
      }

    };
  }
}
