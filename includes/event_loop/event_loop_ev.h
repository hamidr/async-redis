#pragma once

#include <ev.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <queue>

#include "../network/async_socket.hpp"

namespace async_redis {
  namespace event_loop
  {
    class event_loop_ev
    {
      using socket_id = int; //fd;
      using string = std::string;
      struct socket_queue;

    public:
      using async_socket = ::async_redis::network::async_socket;
      using action       = std::function<void()>;

      using socket_identifier_t = std::unordered_map<socket_id, std::unique_ptr<socket_queue>>::iterator;

    private:
      struct timer_watcher
      {
        ev_timer timer;
        action timeout_cb;

        timer_watcher(double time, const action& cb)
          : timeout_cb(cb)
        {
          ev_timer_init (&timer, &event_loop_ev::timer_handler, time, 0.);
        }
      };

      struct socket_queue
      {
        async_socket& socket;
        event_loop_ev& loop_;

        ev_io write_watcher;
        ev_io read_watcher;

        std::queue<action> write_handlers;
        std::queue<action> read_handlers;

        socket_queue(event_loop_ev& loop, int fd, async_socket& s)
          : loop_(loop), socket(s)
        {
          ev_io_init(&read_watcher, &event_loop_ev::read_handler, fd, EV_READ);
          ev_io_init(&write_watcher, &event_loop_ev::write_handler, fd, EV_WRITE);

          write_watcher.data = this;
          read_watcher.data = this;
        }

        ~socket_queue() {
          loop_.stop(write_watcher);
          loop_.stop(read_watcher);
        }
      };

    public:
      event_loop_ev();
      void run();

      socket_identifier_t watch(int, async_socket&);
      void unwatch(socket_identifier_t&);

      void async_write(socket_identifier_t& id, const action& cb);
      void async_read(socket_identifier_t& id, const action& cb);
      void async_timeout(double time, const action& cb );

    private:
      static void read_handler(EV_P_ ev_io* w, int revents);
      static void write_handler(EV_P_ ev_io* w, int revents);
      static void timer_handler(EV_P_ ev_timer* w, int revents);
      void stop(ev_io&);
      void start(ev_io&);

    private:
      struct ev_loop* loop_ = EV_DEFAULT;
      std::unordered_map<socket_id, std::unique_ptr<socket_queue>> watchers_;
    };
  }
}
