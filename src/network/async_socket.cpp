#include "../../includes/network/async_socket.hpp"

#include <sys/fcntl.h> // fcntl
#include <unistd.h> // close

namespace async_redis {
namespace network {

async_socket::async_socket(event_loop::event_loop_ev& io)
  : io_(io)
{ }

async_socket::~async_socket() {
  close();
}

bool async_socket::is_valid() {
  return fd_ != -1;
}

ssize_t async_socket::send(const string& data) {
  return send(data.data(), data.size());
}

ssize_t async_socket::send(const char *data, size_t len) {
  return ::send(fd_, data, len, 0);
}

ssize_t async_socket::receive(char *data, size_t len) {
  return ::recv(fd_, data, len, 0);
}

bool async_socket::listen(int backlog) {
  return ::listen(fd_, backlog) == 0;
}

int async_socket::accept() {
  return ::accept(fd_, nullptr, nullptr);
}

bool async_socket::close()
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

bool async_socket::async_write(const string& data, ready_cb_t fn)
{
  if (!is_connected() || !data.size())
    return false;

  io_.async_write(id_, [this, data, cb{std::move(fn)}]() -> void {
      auto sent_chunk = send(data);

      if(sent_chunk == 0)
        close();

      if (sent_chunk < data.size() && sent_chunk != -1) {
        async_write(data.substr(sent_chunk, data.size()), std::move(cb));
        return;
      }

      cb(sent_chunk);
    });

  return true;
}

bool async_socket::async_read(char *buffer, int max_len, recv_cb_t cb)
{
  if (!is_connected())
    return false;

  io_.async_read(id_, [&, buffer, max_len,  cb{std::move(cb)}]() -> void {
      auto l = receive(buffer, max_len);
      if (l == 0)
        close();

      cb(l);
    });

  return true;
}

void async_socket::async_accept(const std::function<void(std::shared_ptr<async_socket>)>& cb)
{
  return io_.async_read(id_,
    [this, cb]()
    {
      int fd = this->accept();
      auto s = std::make_shared<async_socket>(io_);
      s->set_fd_socket(fd);
      cb(s);
      this->async_accept(cb);
    }
  );
}

bool async_socket::is_connected() const {
  return is_connected_;
}

void async_socket::set_fd_socket(int fd)
{
  fd_ = fd;
  is_connected_ = true;

  id_ = io_.watch(fd_);
}


void async_socket::create_socket(int domain) {
  if (-1 == (fd_ = socket(domain, SOCK_STREAM, 0)))
    throw connect_socket_exception();

  if (-1 == fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK))
    throw nonblocking_socket_exception();

  id_ = io_.watch(fd_);
}

int async_socket::connect_to(async_socket::socket_t* socket_addr, int len)
{
  int ret = ::connect(fd_, socket_addr, len);
  if (!ret)
    is_connected_ = true;

  return ret;
}

int async_socket::bind_to(async_socket::socket_t* socket_addr, int len) {
  int b = ::bind(fd_, socket_addr, len);
  if (!b)
    is_connected_ = true;

  return b;
}

}}
