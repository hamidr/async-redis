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

    struct async_socket
    {
      using recv_cb_t         = std::function<void (const char*, int )>;
      using ready_cb_t        = std::function<void ()>;
      using connect_handler_t = std::function<void (bool)>;

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

    protected:
      int fd_ = -1;
    };

    template <typename InputOutputHanler>
    class async_socket_t : public async_socket
    {
      using event_watcher_t = typename InputOutputHanler::event_watcher_t;

    public:
      async_socket_t(InputOutputHanler& io)
        : io_(io)
      {
      }

      async_socket_t(InputOutputHanler &io, int fd)
        : io_(io)
      {
        fd_ = fd;
        is_connected_ = true;
      }


      inline ~async_socket_t() {
        // io_.remove(fd_, *this);
        close();
      }

      inline
      event_watcher_t async_write(const string& data, const ready_cb_t& cb)
      {
        return io_.async_write(fd_, *this, data, cb);
      }

      inline
      event_watcher_t async_write_then_read(const string& data, const recv_cb_t& cb)
      {
        return this->async_write(data, [&, cb]() {
            this->async_read(cb);
          });
      }

      inline
      event_watcher_t async_read(const recv_cb_t& cb)
      {
        return io_.async_read(fd_, *this, cb);
      }

      template<typename SocketType>
      event_watcher_t async_accept(const std::function<void(std::shared_ptr<SocketType>)>& cb)
      {
        return async_read([&, cb](const char* data, int len){
            int fd = this->accept();
            cb(std::make_shared<SocketType>(io_, fd));
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
    };
  }
}
