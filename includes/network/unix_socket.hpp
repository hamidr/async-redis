#pragma once

#include "async_socket.hpp"

namespace async_redis {
  namespace network
  {
    template<typename InputOutputHanler>
    class unix_socket : public async_socket<InputOutputHanler>
    {
    public:

      inline
      unix_socket(InputOutputHanler &io)
        : async_socket<InputOutputHanler>(io)
      {
        this->create_socket(AF_UNIX);
      }

      inline
      unix_socket(InputOutputHanler &io, int fd)
        : async_socket<InputOutputHanler>(io, fd)
      {}

      int connect(const string& path) {
        struct sockaddr_un addr = {0};
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, path.data());
        auto len = strlen(addr.sun_path) + sizeof(addr.sun_family);

        return this->connect_to((socket_t *)&addr, sizeof(addr));
      }

      bool bind(const string& path) {
        ::unlink(path.data());

        struct sockaddr_un addr = {0};
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, path.data());

        auto len = strlen(addr.sun_path) + sizeof(addr.sun_family);

        return this->bind_to((socket_t *)&addr, sizeof(addr)) == 0;
      }

    };
  }
}
