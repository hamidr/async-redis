#pragma once


#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/fcntl.h> // fcntl
#include <unistd.h> // close
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>

namespace async_redis {
  namespace network
  {
    using socket_t = struct sockaddr;
    using std::string;

    class socket_excetion : std::exception {};
    //TODO: NAMING? More of a permission socket issue than a connect one!
    class connect_socket_exception : socket_excetion {};
    class nonblocking_socket_exception : socket_excetion {};

    template <typename InputOutputHanler>
    class async_socket
    {
    public:
      using SocketIdentifierT  = typename InputOutputHanler::SocketIdentifierT;
      using recv_cb_t         = std::function<void (const char*, int)>;
      using ready_cb_t        = std::function<void ()>;
      using connect_handler_t = std::function<void (bool)>;

      async_socket(InputOutputHanler& io)
        : io_(io)
      { }

      async_socket(InputOutputHanler &io, int fd)
        : io_(io)
      {
        fd_ = fd;
        is_connected_ = true;

        id_ = io_.Watch(fd_);
      }

      inline ~async_socket() {
        close();
        io_.UnWatch(id_);
      }

      inline int send(const string& data) {
        return ::send(fd_, data.data(), data.size(), 0);
      }

      inline int send(const char *data, size_t len) {
        return ::send(fd_, data, len, 0);
      }

      inline int receive(char *data, size_t len) {
        return ::recv(fd_, data, len, 0);
      }

      inline bool listen() {
        return ::listen(fd_, 0) == 0;
      }

      inline int accept() {
        return ::accept(fd_, nullptr, nullptr);
      }

      inline bool close() {
        return ::close(fd_) == 0;
      }


      void async_write(const string& data, const ready_cb_t& cb) {
        return io_.ASyncWrite(id_, [this, data, cb]() {
            send(data);
            cb();
          });
      }

      void async_read(// char* buffer, uint len, 
                      const recv_cb_t& cb) {
        return io_.ASyncRead(id_, [&, // len, 
                                    cb]() {
            char b[1024];
            auto l = receive(b, 1023);
            cb(b, l);
          });
      }

      template <typename SocketType, typename... Args>
      void async_connect(int timeout, async_socket::connect_handler_t handler, Args... args)
      {
        if (timeout == 10) // is equal to 1 second
          return handler(false);

        io_.ASyncTimeout(0.1, [this, timeout, args..., handler]() {

            if (-1 == static_cast<SocketType&>(*this).connect(args...))
              return this->async_connect<SocketType>(timeout+1, handler, args...);

            handler(is_connected());
          });
      }

      template<typename SocketType>
      void async_accept(const std::function<void(std::shared_ptr<SocketType>)>& cb)
      {
        return io_.ASyncRead(id_, [&, cb]() {
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

        id_ = io_.Watch(fd_);
      }

      int connect_to(socket_t* socket_addr, int len) {
        int ret = ::connect(fd_, socket_addr, len);
        if (!ret)
          is_connected_ = true;

        return ret;
      }

      int bind_to(socket_t* socket_addr, int len) {
        return ::bind(fd_, socket_addr, len);
      }

    private:
      bool is_connected_ = false;
      InputOutputHanler& io_;
      SocketIdentifierT id_;
      int fd_ = -1;
    };
  }
}
