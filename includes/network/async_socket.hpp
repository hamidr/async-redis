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
    using std::string;

    class socket_excetion : std::exception {};
    //TODO: NAMING? More of a permission socket issue than a connect one!
    class connect_socket_exception : socket_excetion {};
    class nonblocking_socket_exception : socket_excetion {};

    class async_socket
    {
    public:
      using recv_cb_t         = std::function<void (const char*, int )>;
      using ready_cb_t        = std::function<void ()>;
      using connect_handler_t = std::function<void (bool)>;

      inline async_socket() {
      }

      inline ~async_socket() {
        close();
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

      template <typename InputOutputHanler>
      typename InputOutputHanler::event_watcher_t
        async_write(InputOutputHanler &io, const string& data, const ready_cb_t& cb)
      {
        using watcher = typename InputOutputHanler::event_watcher;
        return io.async_write(fd_, *this, data, [cb](watcher&) {
            cb();
          });
      }

      template <typename InputOutputHanler, typename event_watcher = typename InputOutputHanler::event_watcher_t>
      event_watcher async_write_then_read(InputOutputHanler &io, const string& data, const recv_cb_t& cb)
      {
        using watcher = typename InputOutputHanler::event_watcher;

        return io.async_write(fd_, *this, data, [&, cb](watcher& w) {
            io.switch_read(w, [&](watcher&, const char* chunk, int len) {
                cb(chunk, len);
              });
          });
      }

      template <typename InputOutputHanler>
      typename InputOutputHanler::event_watcher_t
      async_read(InputOutputHanler &io, const recv_cb_t& cb)
      {
        using watcher = typename InputOutputHanler::event_watcher;

        return io.async_read(fd_, *this, [&, cb](watcher&, const char* chunk, int len) {
            cb(chunk, len);
          });
      }

      template <typename InputOutputHanler, typename SocketType>
      typename InputOutputHanler::event_watcher_t
        async_accept(InputOutputHanler &io, const std::function<void(std::shared_ptr<SocketType>)>& cb)
      {
        // LOG_THIS
        using watcher = typename InputOutputHanler::event_watcher;
        return io.async_read(fd_, *this, [&, cb](watcher&, const char* data, int len){
            // LOG_THIS
            int fd = this->accept();
            cb(std::make_shared<SocketType>(fd));
          });
      }


      bool is_connected() const {
        return is_connected_;
      }

    protected:
      using socket_t = struct sockaddr;

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

      async_socket(int fd) {
        fd_ = fd;
        is_connected_ = true;
      }

    private:
      int fd_ = -1;
      bool is_connected_ = false;
    };
  }
}
