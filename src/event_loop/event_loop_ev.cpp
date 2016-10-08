#include "../../includes/event_loop/event_loop_ev.h"

namespace async_redis {
namespace event_loop {

event_loop_ev::event_loop_ev()
{
}

void event_loop_ev::run()
{
  ev_run (loop_, 0);
}

void event_loop_ev::disconnect(async_socket& socket)
{
  socket.close();
}

event_loop_ev::event_watcher_t event_loop_ev::async_write(int fd, async_socket& socket, const string& data, const ready_cb_t& cb)
{
  /* LOG_THIS; */
  auto &&ptr = std::make_unique<event_watcher>(fd, *this, socket, data, cb);
  start(ptr->io);
  return std::move(ptr);
}

event_loop_ev::event_watcher_t event_loop_ev::async_read(int fd, async_socket& socket, const recv_cb_t& cb)
{
  /* LOG_THIS; */
  auto &&ptr = std::make_unique<event_watcher>(fd, *this, socket, cb);
  start(ptr->io);
  return std::move(ptr);
}

void event_loop_ev::switch_read(event_watcher& watcher, const recv_cb_t& recv)
{
  /* LOG_THIS; */
  stop(watcher.io);
  watcher.recv_cb = recv;
  ev_io_init(&watcher.io, &event_loop_ev::read_handler, watcher.io.fd, EV_READ);
  start(watcher.io);
}

void event_loop_ev::async_timeout(int time, const timeout_cb_t& cb )
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

  event_watcher* event = reinterpret_cast<event_watcher*>(w);

  char tmp_buf[256] = {0};
  auto received_len = event->socket.receive(tmp_buf, 255);

  event->recv_cb(*event, tmp_buf, received_len);
}

void event_loop_ev::write_handler(EV_P_ ev_io* w, int revents)
{
  /* LOG_THIS; */
  if (!(revents & EV_WRITE)) {
    // LOG_ERR("WRONG EVENT ON read_handler");
    return;
  }

  event_watcher* event = reinterpret_cast<event_watcher*>(w);
  event->socket.send(event->write);

  event->done_cb(*event);
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

}}
