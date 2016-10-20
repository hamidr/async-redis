#pragma once

#include <ev.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <queue>

namespace async_redis {
  namespace event_loop
  {
    class event_loop_ev
    {
      using socket_id = int;
      using string = std::string;
      struct socket_queue;

    public:
      using action              = std::function<void()>;
      using socket_identifier_t = socket_queue *;

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
        event_loop_ev& loop_;

        ev_io write_watcher;
        ev_io read_watcher;
        bool free_me = false;

        std::queue<action> write_handlers;
        std::queue<action> read_handlers;

        socket_queue(event_loop_ev& loop, int fd)
          : loop_(loop)
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
      event_loop_ev(struct ev_loop *);
      void run();

      socket_identifier_t watch(int);
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
      struct ev_loop* loop_;
    };
  }
}
