#include <co/adro/network/async_socket.h>
#include <co/adro/event_loop/event_loop_ev.h>

namespace co{
namespace adro{
namespace network{

AsyncSocket::AsyncSocket(co::adro::event_loop::EventLoopEV& io)
  : io_(io)
{ 

}

AsyncSocket::AsyncSocket(co::adro::event_loop::EventLoopEV &io, int fd)
  : io_(io)
{
  fd_ = fd;
  isConnected_ = true;

  id_ = io_.Watch(fd_);
}


AsyncSocket::~AsyncSocket() 
{
  Close();
  io_.UnWatch(id_);
}

int 
AsyncSocket::Send(const std::string& data) 
{
  return ::send(fd_, data.data(), data.size(), 0);
}

int 
AsyncSocket::Send(const char *data, size_t len) 
{
  return ::send(fd_, data, len, 0);
}

int 
AsyncSocket::Receive(char *data, size_t len) 
{
  return ::recv(fd_, data, len, 0);
}

inline bool 
AsyncSocket::Listen() 
{
  return ::listen(fd_, 0) == 0;
}

int 
AsyncSocket::Accept() 
{
  return ::accept(fd_, nullptr, nullptr);
}

bool 
AsyncSocket::Close() 
{
  return ::close(fd_) == 0;
}

void 
AsyncSocket::AsyncWrite(const std::string& data, const std::function<void()>& cb) 
{
  return io_.ASyncWrite(id_, [this, data, cb]() {
      Send(data);
      cb();
    });
}

void 
AsyncSocket::AsyncRead(/* char* buffer, uint len, */const std::function<void(const char*, int)>& cb) 
{
  return io_.ASyncRead(id_, [&, /* len, */ cb]() {
      char b[1024];
      auto l = Receive(b, 1023);
      cb(b, l);
    });
}


//FIXME TODO implement connect with try number counter and write permision on socket
template <typename SocketType, typename... Args>
void 
AsyncSocket::AsyncConnect(int timeout, std::function<void(bool)> handler, Args... args)
{
  if (timeout == 10) // is equal to 1 second
    return handler(false);

  io_.ASyncTimeout(0.1, [this, timeout, args..., handler]() {

      if (-1 == static_cast<SocketType&>(*this).connect(args...))
        return this->AsyncConnect<SocketType>(timeout+1, handler, args...);

      handler(IsConnected());
    });
}

template <typename SocketType>
void 
AsyncSocket::AsyncAccept(const std::function<void(std::shared_ptr<SocketType>)>& cb)
{
  return io_.ASyncRead(id_, [&, cb]() {
      int fd = this->Accept();
      cb(std::make_shared<SocketType>(io_, fd));
      this->AsyncAccept(cb);
    });
}

bool 
AsyncSocket::IsConnected() const 
{
  return isConnected_;
}

void 
AsyncSocket::CreateSocket(int domain) 
{
  if (-1 == (fd_ = socket(domain, SOCK_STREAM, 0)))
    throw std::runtime_error("Can't Create socket. errno:");

  if (-1 == fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL) | O_NONBLOCK))
    throw std::runtime_error("Can't change file descriptor to nonblock mode. errno:");

  id_ = io_.Watch(fd_);
}

int 
AsyncSocket::ConnectTo(struct sockaddr* socket_addr, int len) {
  int ret = ::connect(fd_, socket_addr, len);
  if (!ret)
    isConnected_ = true;
  return ret;
}

int 
AsyncSocket::BindTo(struct sockaddr* socket_addr, int len) {
  return ::bind(fd_, socket_addr, len);
}


}
}
}