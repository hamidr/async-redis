#pragma once

#include <sys/socket.h>

#include <string>
#include <event_loop/event_loop_ev.h>

namespace async_redis {
  namespace network
  {
    using std::string;

    class socket_excetion : std::exception {};
    //TODO: NAMING? More of a permission socket issue than a connect one!
    class connect_socket_exception : socket_excetion {};
    class nonblocking_socket_exception : socket_excetion {};

    class async_socket
    {
    public:
      using socket_t = struct sockaddr;

      using socket_identifier_t  = event_loop::event_loop_ev::socket_identifier_t;
      using recv_cb_t         = std::function<void (ssize_t)>;
      using ready_cb_t        = std::function<void (ssize_t)>;
      using connect_handler_t = std::function<void (bool)>;

      async_socket(event_loop::event_loop_ev& io);

      ~async_socket();

      bool is_valid();
      ssize_t send(const string& data);
      ssize_t send(const char *data, size_t len);
      ssize_t receive(char *data, size_t len);
      bool listen(int backlog = 0);
      int accept();
      bool close();
      bool async_write(const string& data, const ready_cb_t& cb);
      bool async_read(char *buffer, int max_len, const recv_cb_t& cb);
      void async_accept(const std::function<void(std::shared_ptr<async_socket>)>& cb);

      bool is_connected() const;

    protected:
      void set_fd_socket(int fd);

      template <typename SocketType, typename... Args>
      void async_connect(int timeout, connect_handler_t handler, Args... args)
      {
        if (timeout == 10) // is equal to 1 second
          return handler(false);

        io_.async_timeout(0.1, [this, timeout, args..., handler]() {

            if (-1 == static_cast<SocketType&>(*this).connect(args...))
              return this->async_connect<SocketType>(timeout+1, handler, args...);

            handler(is_connected());
          });
      }

      void create_socket(int domain);
      int connect_to(socket_t* socket_addr, int len);
      int bind_to(socket_t* socket_addr, int len);

    private:
      bool is_connected_ = false;
      event_loop::event_loop_ev& io_;
      socket_identifier_t id_;
      int fd_ = -1;
    };
  }
}
