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

    public:
      using socket_id = int; //fd;
      using string = std::string;

      using event_watcher_t = int;
      using async_socket    = ::async_redis::network::async_socket;
      using timeout_cb_t    = std::function<void()>;
      using ready_cb_t      = async_socket::ready_cb_t;
      using recv_cb_t       = async_socket::recv_cb_t;

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

      struct socket_queue
      {
        async_socket& socket;
        event_loop_ev& loop_;

        ev_io write_watcher;
        ev_io read_watcher;

        std::queue<std::tuple<const string&, ready_cb_t>> write_handlers;
        std::queue<recv_cb_t> read_handlers;

        socket_queue(event_loop_ev& loop, async_socket& s)
          : loop_(loop), socket(s)
        {
          write_watcher.data = this;
          read_watcher.data = this;
        }

        ~socket_queue() {
          loop_.stop(write_watcher);
          loop_.stop(read_watcher);
        }
      };



      event_loop_ev();
      void run();

      template <typename SocketType, typename... Args>
      void connect(SocketType& socket, async_socket::connect_handler_t handler, Args... args) {
        async_timeout(1, [this, &socket, &args..., &handler]() {
            if (-1 == socket.connect(args...))
              connect(socket, handler, args...);

            handler(socket.is_connected());
          });
      }

      void disconnect(async_socket& socket);
      void stop(event_watcher_t& w);

      event_watcher_t async_write(int fd, async_socket& socket, const string& data, const ready_cb_t& cb);
      event_watcher_t async_read(int fd, async_socket& socket, const recv_cb_t& cb);
      void async_timeout(int time, const timeout_cb_t& cb );

    private:
      static void read_handler(EV_P_ ev_io* w, int revents);
      static void write_handler(EV_P_ ev_io* w, int revents);
      static void timer_handler(EV_P_ ev_timer* w, int revents);
      void stop(ev_io&);
      void start(ev_io&);
      socket_queue* find_or_insert_watcher(socket_id& id, async_socket& socket);

    private:
      struct ev_loop* loop_ = EV_DEFAULT;
      std::unordered_map<socket_id, std::unique_ptr<socket_queue>> watchers_;
    };
  }
}
