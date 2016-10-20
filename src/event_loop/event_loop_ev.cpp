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

void event_loop_ev::async_write(socket_identifier_t& watcher, const action& cb)
{
  auto &handlers = watcher->write_handlers;
  handlers.push(cb);

  if (watcher->write_handlers.size() == 1) {
    ev_io *w = &watcher->write_watcher;
    ev_io_start(loop_, w);
  }
}

void event_loop_ev::async_read(socket_identifier_t& watcher, const action& cb)
{
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

void event_loop_ev::read_handler(EV_P_ ev_io* w, int revents)
{
  if (!(revents & EV_READ)) {
    // LOG_ERR("WRONG EVENT ON read_handler");
    return;
  }

  socket_queue* sq = reinterpret_cast<socket_queue*>(w->data);

  if (sq->free_me) {
    delete sq;
    return;
  }

  auto &handlers = sq->read_handlers;

  if (handlers.size() != 0)
  {
    auto &action = handlers.front();
    action();
    handlers.pop();
  }

  if (handlers.size() == 0)
    ev_io_stop(loop, &sq->read_watcher);

  if (sq->free_me)
    delete sq;
}

void event_loop_ev::write_handler(EV_P_ ev_io* w, int revents)
{
  if (!(revents & EV_WRITE)) {
    // LOG_ERR("WRONG EVENT ON read_handler");
    return;
  }

  socket_queue* sq = reinterpret_cast<socket_queue*>(w->data);

  if (sq->free_me) {
    delete sq;
    return;
  }

  auto &handlers = sq->write_handlers;

  if (handlers.size() != 0)
  {
    auto &action = handlers.front();
    action();
    handlers.pop();
  }

  if (handlers.size() == 0)
    ev_io_stop(loop, &sq->write_watcher);

  if (sq->free_me)
    delete sq;
}

void event_loop_ev::timer_handler(EV_P_ ev_timer* w, int revents)
{
  timer_watcher *watcher = reinterpret_cast<timer_watcher*>(w);
  watcher->timeout_cb();
  delete watcher;
}

void event_loop_ev::stop(ev_io& io)
{
  ev_clear_pending(loop_, &io);
  ev_io_stop(loop_, &io);
}

void event_loop_ev::start(ev_io& io)
{
  ev_io_start(loop_, &io);
}

event_loop_ev::socket_identifier_t event_loop_ev::watch(int fd)
{
  socket_identifier_t ptr = new event_loop_ev::socket_queue(*this, fd);
  return ptr;
}

void event_loop_ev::unwatch(socket_identifier_t& id)
{
  id->free_me = true;
}

}}
