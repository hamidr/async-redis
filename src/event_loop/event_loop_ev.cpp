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

void event_loop_ev::async_write(socket_identifier_t& id, const action& cb)
{
  socket_queue *watcher = id->second.get();

  auto &handlers = watcher->write_handlers;
  handlers.push(cb);

  if (watcher->write_handlers.size() == 1) {
    ev_io *w = &watcher->write_watcher;
    ev_io_start(loop_, w);
  }
}

void event_loop_ev::async_read(socket_identifier_t& id, const action& cb)
{
  socket_queue *watcher = id->second.get();

  auto &handlers = watcher->read_handlers;
  handlers.push(cb);

  if (watcher->read_handlers.size() == 1) {
    ev_io *w = &watcher->read_watcher;
    ev_io_start(loop_, w);
  }
}

void event_loop_ev::async_timeout(double time, const action& cb )
{
  timer_watcher *w = new timer_watcher(time, cb);
  ev_timer_start (loop_, &w->timer);
}

void event_loop_ev::read_handler(EV_P_ ev_io* w, int revents) {
  /* LOG_THIS; */
  if (!(revents & EV_READ)) {
    // LOG_ERR("WRONG EVENT ON read_handler");
    return;
  }

  socket_queue* sq = reinterpret_cast<socket_queue*>(w->data);
  auto &handlers = sq->read_handlers;

  if (handlers.size() != 0)
  {
    auto &action = handlers.front();
    action();
    handlers.pop();
  }

  if (handlers.size() == 0)
    ev_io_stop(loop, &sq->read_watcher);
}

void event_loop_ev::write_handler(EV_P_ ev_io* w, int revents)
{
  /* LOG_THIS; */
  if (!(revents & EV_WRITE)) {
    // LOG_ERR("WRONG EVENT ON read_handler");
    return;
  }

  socket_queue* sq = reinterpret_cast<socket_queue*>(w->data);
  auto &handlers = sq->write_handlers;

  if (handlers.size() != 0)
  {
    auto &action = handlers.front();
    action();
    handlers.pop();
  }

  if (handlers.size() == 0)
    ev_io_stop(loop, &sq->write_watcher);
}

void event_loop_ev::timer_handler(EV_P_ ev_timer* w, int revents)
{
  timer_watcher *watcher = reinterpret_cast<timer_watcher*>(w);
  watcher->timeout_cb();
  delete watcher;
}

void event_loop_ev::stop(ev_io& io)
{
  /* LOG_THIS; */
  ev_io_stop(loop_, &io);
}

void event_loop_ev::start(ev_io& io)
{
  /* LOG_THIS; */
  ev_io_start(loop_, &io);
}

event_loop_ev::socket_identifier_t event_loop_ev::watch(int fd)
{
  auto iter = watchers_.find(fd);

  if (iter == watchers_.end()) {
    auto w = watchers_.emplace(fd, std::make_unique<event_loop_ev::socket_queue>(*this, fd));
    return w.first;
  }

  return iter;
}

void event_loop_ev::unwatch(socket_identifier_t& id)
{
  watchers_.erase(id);
}

}}
