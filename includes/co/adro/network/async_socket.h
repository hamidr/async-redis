#pragma once
#include <iostream>
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
#include <co/adro/event_loop/event_loop_ev.h>
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

      int Send(const std::string& data);
      int Send(const char *data, size_t len);
      int Receive(char *data, size_t len);
      bool Listen();
      int Accept();
      bool Close();
      bool IsConnected() const;

      virtual int Connect(const std::string& host, int port) = 0 ;
      virtual bool Bind(const std::string& host, int port) = 0 ; 


      void AsyncWrite(const std::string& data, const std::function<void()>& cb);
      void AsyncRead(/* char* buffer, uint len, */const std::function<void(const char*, int)>& cb);
      void AsyncConnect(int timeout, std::function<void(bool) > handler,std::string host , int port) 
      {
        if (timeout == 10) // is equal to 1 second
          return handler(false);

        io_.ASyncTimeout(0.1, [this, timeout,host , port , handler]() {

            if (-1 == static_cast<AsyncSocket&>(*this).Connect(host, port ))
              return this->AsyncConnect(timeout+1, handler, host , port );

            handler(this->IsConnected());
          });
      }


      template<typename SocketType>
      void AsyncAccept(const std::function<void(std::shared_ptr<SocketType>)>& cb)
      {
        return io_.ASyncRead(id_, [&, cb]() {
            int fd = this->Accept();
            cb(std::make_shared<SocketType>(io_, fd));
            this->AsyncAccept(cb);
          });
      }



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
