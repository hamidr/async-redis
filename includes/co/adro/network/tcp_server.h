#pragma once

#include <string>
#include <memory>
#include <functional>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace co{
namespace adro{
namespace event_loop{
  class EventLoopEV; 
}
namespace network{
  class TCPSocket;

  class TCPServer
  {
  public:
    TCPServer(event_loop::EventLoopEV& loop);
    void Listen(int port) ;
    void Accept(std::shared_ptr<TCPSocket> socket);
  private:
    void ChunkReceived(const char* data, int len, std::shared_ptr<TCPSocket>& socket);
  private:
    std::shared_ptr<TCPSocket> listener_;
    event_loop::EventLoopEV& loop_;
    std::unordered_map<std::shared_ptr<TCPSocket>, void*> conns_;
  };

}
}
}
