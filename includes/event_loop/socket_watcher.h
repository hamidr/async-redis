#pragma once

#include <ev.h>
#include <queue>
#include <functional>

namespace async_redis {
namespace event_loop {

typedef std::function<void()> action;

class socket_watcher
{
 public:
  socket_watcher(struct ev_loop* loop, int fd);

  void stop();

  void start_reading_with(action&& cb);
  void start_writing_with(action&& cb);

 private:
  void call_read();
  void call_write();

  static void read_handler(EV_P_ ev_io* w, int revents);
  static void write_handler(EV_P_ ev_io* w, int revents);

 private:
  void start_reading();
  void start_writing();

  void stop_reading();
  void stop_writing();

 private:
  struct ev_loop* loop_;

  ev_io write_watcher_;
  ev_io read_watcher_;

  std::queue<action> write_handlers_;
  std::queue<action> read_handlers_;
};

}
}
