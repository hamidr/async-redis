#include "../../includes/event_loop/socket_watcher.h"

namespace async_redis {
namespace event_loop {

socket_watcher::socket_watcher(struct ev_loop* loop, int fd)
  : loop_(loop)
{
  ev_io_init(&read_watcher_, &socket_watcher::read_handler, fd, EV_READ);
  ev_io_init(&write_watcher_, &socket_watcher::write_handler, fd, EV_WRITE);

  write_watcher_.data = this;
  read_watcher_.data = this;
}

  void socket_watcher::start_writing_with(action&& cb)
{
  write_handlers_.push(std::move(cb));

  if (write_handlers_.size() != 1)
    return;

  start_writing();
}

void socket_watcher::start_reading_with(action&& cb)
{
  read_handlers_.push(std::move(cb));

  if (read_handlers_.size() != 1)
    return;

  start_reading();
}

void socket_watcher::call_read()
{
  if (read_handlers_.size())
  {
    auto &action = read_handlers_.front();
    action();
    read_handlers_.pop();
  }

  if (!read_handlers_.size())
    stop_reading();
}


void socket_watcher::call_write()
{
  if (write_handlers_.size())
    {
      auto &action = write_handlers_.front();
      action();
      write_handlers_.pop();
    }

  if (!write_handlers_.size())
    stop_writing();
}

void socket_watcher::write_handler(EV_P_ ev_io* w, int revents)
{
  if (revents & EV_ERROR)
    return;

  socket_watcher* sq = reinterpret_cast<socket_watcher*>(w->data);
  sq->call_write();
}

void socket_watcher::read_handler(EV_P_ ev_io* w, int revents)
{
  if (revents & EV_ERROR)
    return;

  socket_watcher* sq = reinterpret_cast<socket_watcher*>(w->data);
  sq->call_read();
}


void socket_watcher::stop_reading()
{
  ev_clear_pending(loop_, &read_watcher_);
  ev_io_stop(loop_, &read_watcher_);
}

void socket_watcher::stop_writing()
{
  ev_clear_pending(loop_, &write_watcher_);
  ev_io_stop(loop_, &write_watcher_);
}

void socket_watcher::start_writing()
{
  ev_io_start(loop_, &write_watcher_);
}

void socket_watcher::start_reading()
{
  ev_io_start(loop_, &read_watcher_);
}

void socket_watcher::stop()
{
  stop_writing();
  stop_reading();
}

}
}
