#pragma once

#include <ev.h>
#include <functional>
#include <memory>

#include "../network/async_socket.hpp"

namespace async_redis {
  namespace event_loop
  {
    class event_loop_ev
    {
    public:
      struct event_watcher;

      using event_watcher_t = std::unique_ptr<event_watcher>;
      using ready_cb_t      = std::function<void(event_watcher&)>;
      using recv_cb_t       = std::function<void(event_watcher&, const char*, int)>;
      using timeout_cb_t    = std::function<void()>;

      using async_socket = ::async_redis::network::async_socket;
      using string = std::string;

      struct event_watcher
      {
        ev_io io;
        event_loop_ev& loop_;
        async_socket& socket;
        const string write;
        ready_cb_t done_cb;
        recv_cb_t recv_cb;

        event_watcher(int fd, event_loop_ev& loop, async_socket& socket, const string& data, const ready_cb_t& done)
          : loop_(loop), socket(socket), done_cb(done), write(data)
        {
          ev_io_init(&io, &event_loop_ev::write_handler, fd, EV_WRITE);
        }

        event_watcher(int fd, event_loop_ev& loop, async_socket& socket, const recv_cb_t& recv)
          : loop_(loop), socket(socket), recv_cb(recv)
        {
          ev_io_init(&io, &event_loop_ev::read_handler, fd, EV_READ);
        }

        ~event_watcher() {
          loop_.stop(io);
        }
      };

      struct timer_watcher
      {
        ev_timer timer;
        timeout_cb_t timeout_cb;

        timer_watcher(int time, const timeout_cb_t& cb)
          : timeout_cb(cb)
        {
          ev_timer_init (&timer, &event_loop_ev::timer_handler, time, 0.);
        }
      };


      event_loop_ev();
      void run();

      template <typename SocketType, typename... Args>
      void connect(SocketType& socket, async_socket::connect_handler_t handler, Args... args) {
        while(-1 == socket.connect(args...))
          printf("helllo");

        //return socket and then call it!
        handler(socket.is_connected());
      }

      void disconnect(async_socket& socket);

      event_watcher_t async_write(int fd, async_socket& socket, const string& data, const ready_cb_t& cb);
      event_watcher_t async_read(int fd, async_socket& socket, const recv_cb_t& cb);
      void switch_read(event_watcher& watcher, const recv_cb_t& recv);
      void async_timeout(int time, const timeout_cb_t& cb );

    private:
      static void read_handler(EV_P_ ev_io* w, int revents);
      static void write_handler(EV_P_ ev_io* w, int revents);
      static void timer_handler(EV_P_ ev_timer* w, int revents);
      void stop(ev_io& io);
      void start(ev_io& io);

    private:
      struct ev_loop* loop_ = EV_DEFAULT;
    };
  }
}
