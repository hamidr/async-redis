#pragma once


#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/fcntl.h> // fcntl
#include <unistd.h> // close
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include <event_loop/event_loop_ev.h>

namespace async_redis {
  namespace network
  {
    using socket_t = struct sockaddr;
    using std::string;

    class socket_excetion : std::exception {};
    //TODO: NAMING? More of a permission socket issue than a connect one!
    class connect_socket_exception : socket_excetion {};
    class nonblocking_socket_exception : socket_excetion {};

    class async_socket
    {
    public:

      using socket_identifier_t  = event_loop::event_loop_ev::socket_identifier_t;
      using recv_cb_t         = std::function<void (ssize_t)>;
      using ready_cb_t        = std::function<void (ssize_t)>;
      using connect_handler_t = std::function<void (bool)>;

      async_socket(event_loop::event_loop_ev& io)
        : io_(io)
      { }

      async_socket(event_loop::event_loop_ev &io, int fd)
        : io_(io)
      {
        fd_ = fd;
        is_connected_ = true;

        id_ = io_.watch(fd_);
      }

      ~async_socket() {
        close();
      }

      inline bool is_valid() {
        return fd_ != -1;
      }

      inline ssize_t send(const string& data) {
        return send(data.data(), data.size());
      }

      inline ssize_t send(const char *data, size_t len) {
        return ::send(fd_, data, len, 0);
      }

      inline ssize_t receive(char *data, size_t len) {
        return ::recv(fd_, data, len, 0);
      }

      inline bool listen(int backlog = 0) {
        return ::listen(fd_, backlog) == 0;
      }

      inline int accept() {
        return ::accept(fd_, nullptr, nullptr);
      }

      bool close()
      {
        if (!is_connected_)
          return true;

        if(id_)
          io_.unwatch(id_);

        auto res = ::close(fd_) == 0;
        is_connected_ = false;
        fd_ = -1;
        return res;
      }

      bool async_write(const string& data, const ready_cb_t& cb)
      {
        if (!is_connected() || !data.size())
          return false;

        io_.async_write(id_, [this, data, cb]() -> void {
            auto sent_chunk = send(data);

            if(sent_chunk == 0)
              close();

            if (sent_chunk < data.size() && sent_chunk != -1) {
              async_write(data.substr(sent_chunk, data.size()), cb);
              return;
            }

            cb(sent_chunk);
          });

        return true;
      }

      bool async_read(char *buffer, int max_len, const recv_cb_t& cb)
      {
        if (!is_connected())
          return false;

        io_.async_read(id_, [&, buffer, max_len,  cb]() -> void {
            auto l = receive(buffer, max_len);
            if (l == 0)
              close();

            cb(l);
          });

        return true;
      }

      template <typename SocketType, typename... Args>
      void async_connect(int timeout, async_socket::connect_handler_t handler, Args... args)
      {
        if (timeout == 10) // is equal to 1 second
          return handler(false);

        io_.async_timeout(0.1, [this, timeout, args..., handler]() {

            if (-1 == static_cast<SocketType&>(*this).connect(args...))
              return this->async_connect<SocketType>(timeout+1, handler, args...);

            handler(is_connected());
          });
      }

      template<typename SocketType>
      void async_accept(const std::function<void(std::shared_ptr<SocketType>)>& cb)
      {
        return io_.async_read(id_, [&, cb]() {
            int fd = this->accept();
            cb(std::make_shared<SocketType>(io_, fd));
            this->async_accept(cb);
          });
      }

      inline
      bool is_connected() const {
        return is_connected_;
      }

    protected:
      void create_socket(int domain) {
        if (-1 == (fd_ = socket(domain, SOCK_STREAM, 0)))
          throw connect_socket_exception();

        if (-1 == fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK))
          throw nonblocking_socket_exception();
      }

      //TODO: well i guess retry with create_socket in these functions
      int connect_to(socket_t* socket_addr, int len) {
        int ret = ::connect(fd_, socket_addr, len);
        if (!ret) {
          id_ = io_.watch(fd_);
          is_connected_ = true;
        }

        return ret;
      }

      int bind_to(socket_t* socket_addr, int len) {
        return ::bind(fd_, socket_addr, len);
      }

    private:
      bool is_connected_ = false;
      event_loop::event_loop_ev& io_;
      socket_identifier_t id_;
      int fd_ = -1;
    };
  }
}
