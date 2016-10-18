#pragma once

#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/fcntl.h> // fcntl
#include <unistd.h> // close
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <functional> 
#include <memory>
#include <unordered_map>

namespace co{
namespace adro{
namespace event_loop{
  class EventLoopEV;
  class IOWatcher; 
}
namespace network {
    class AsyncSocket
    {
      using SocketIdentifierT = std::unordered_map<int, std::unique_ptr<event_loop::IOWatcher>>::iterator;
    public:
      AsyncSocket(co::adro::event_loop::EventLoopEV& io);
      AsyncSocket(co::adro::event_loop::EventLoopEV&io, int fd);

      ~AsyncSocket();

      inline int Send(const std::string& data);
      inline int Send(const char *data, size_t len);
      inline int Receive(char *data, size_t len);
      inline bool Listen();
      inline int Accept();
      inline bool Close();
      inline bool IsConnected() const;

      void AsyncWrite(const std::string& data, const std::function<void()>& cb);
      void AsyncRead(/* char* buffer, uint len, */const std::function<void(const char*, int)>& cb);

      template <typename SocketType, typename... Args>
      void AsyncConnect(int timeout, std::function<void(bool) > handler, Args... args); 

      template<typename SocketType>
      void AsyncAccept(const std::function<void(std::shared_ptr<SocketType>)>& cb);

    protected:
      void CreateSocket(int domain) ;
      int ConnectTo(struct sockaddr* socket_addr, int len) ;
      int BindTo(struct sockaddr* socket_addr, int len) ;

    private:
      bool isConnected_ = false;
      co::adro::event_loop::EventLoopEV& io_;
      SocketIdentifierT id_;
      int fd_ = -1;
    };




}
}
}
