#pragma once

#include <event_loop/socket_watcher.h>
#include <memory>

namespace async_redis {
  namespace event_loop
  {
    class event_loop_ev
    {
    public:
      using socket_identifier_t = std::shared_ptr<socket_watcher>;

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


    public:
      event_loop_ev();
      event_loop_ev(struct ev_loop *);

      void run();

      socket_identifier_t watch(int);
      void unwatch(socket_identifier_t&);

      void async_write(socket_identifier_t& id, action&& cb);
      void async_read(socket_identifier_t& id, action&& cb);
      void async_timeout(double time, action&& cb );

    private:
      static void timer_handler(EV_P_ ev_timer* w, int revents);

    private:
      struct ev_loop* loop_;
    };
  }
}
