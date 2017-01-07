#include "../../includes/event_loop/event_loop_ev.h"

namespace async_redis {
namespace event_loop {

event_loop_ev::event_loop_ev()
  : loop_(EV_DEFAULT)
{
}

event_loop_ev::event_loop_ev(struct ev_loop* loop)
  : loop_(loop)
{
}

void event_loop_ev::run()
{
  ev_run (loop_, 0);
}

void event_loop_ev::async_write(socket_identifier_t& watcher, action&& cb)
{
  watcher->start_writing_with(std::move(cb));
}

void event_loop_ev::async_read(socket_identifier_t& watcher, action&& cb)
{
  watcher->start_reading_with(std::move(cb));
}

void event_loop_ev::async_timeout(double time, action&& cb )
{
  timer_watcher *w = new timer_watcher(time, cb);
  ev_timer_start (loop_, &w->timer);
}

void event_loop_ev::timer_handler(EV_P_ ev_timer* w, int revents)
{
  timer_watcher *watcher = reinterpret_cast<timer_watcher*>(w);
  watcher->timeout_cb();
  delete watcher;
}


event_loop_ev::socket_identifier_t event_loop_ev::watch(int fd)
{
  return std::make_shared<socket_watcher>(loop_, fd);
}

void event_loop_ev::unwatch(socket_identifier_t& id)
{
  id->stop();
}

}}
