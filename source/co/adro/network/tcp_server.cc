#include <co/adro/network/tcp_server.h>
#include <co/adro/network/tcp_socket.h>
#include <co/adro/event_loop/event_loop_ev.h>


namespace co{
namespace adro{
namespace network{

TCPServer::TCPServer(event_loop::EventLoopEV& loop)
  : loop_(loop) 
{
  listener_ = std::make_shared<TCPSocket>(loop_);
}

void 
TCPServer::Listen(int port) 
{
  if (!listener_->Bind("127.0.0.1", port) || !listener_->Listen())
    throw;
  auto receiver = std::bind(&TCPServer::Accept, this, std::placeholders::_1);
  listener_->template AsyncAccept<TCPSocket>(receiver);
}

void 
TCPServer::Accept(std::shared_ptr<TCPSocket> socket) 
{
  auto receiver = std::bind(&TCPServer::ChunkReceived, this, std::placeholders::_1, std::placeholders::_2, socket);
  socket->AsyncRead(receiver);
  conns_.emplace(socket, nullptr);
}

void 
TCPServer::ChunkReceived(const char* data, int len, std::shared_ptr<TCPSocket>& socket) 
{
  ssize_t acc = 0;
  bool is_finished = false;
  if (len == 0) {
    conns_.erase(socket);
    return;
  }
  socket->AsyncWrite("hello world!", [this, &socket]() {
    loop_.ASyncTimeout(.1, [this, &socket]() {
      conns_.erase(socket);
    });
  });
}


}
}
}
