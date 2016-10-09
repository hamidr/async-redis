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

event_loop_ev::socket_queue* event_loop_ev::find_or_insert_watcher(socket_id& fd, async_socket& socket) {
  socket_queue *watcher;
  auto iter = watchers_.find(fd);

  if (iter == watchers_.end()) {
    // auto w = watchers_[fd] = 
      std::make_unique<event_loop_ev::socket_queue>(*this, socket);
    // return w.second.data();
  }

  return iter->second.get();
}

event_loop_ev::event_watcher_t event_loop_ev::async_write(int fd, async_socket& socket, const string& data, const ready_cb_t& cb)
{
  socket_queue *watcher = find_or_insert_watcher(fd, socket);

  watcher->write_handlers.push(std::make_tuple(data, cb));

  if (watcher->write_handlers.size() == 1) {
    ev_io *w = &watcher->write_watcher;
    ev_io_init(w, &event_loop_ev::write_handler, fd, EV_WRITE);
    ev_io_start(loop_, w);
  }
  return 0;
  //TODO: clean it

  // return watcher->write_handlers.begin();
}

event_loop_ev::event_watcher_t event_loop_ev::async_read(int fd, async_socket& socket, const recv_cb_t& cb)
{
  socket_queue *watcher = find_or_insert_watcher(fd, socket);

  watcher->read_handlers.push(cb);
  if (watcher->read_handlers.size() == 1) {
    ev_io *w = &watcher->read_watcher;
    ev_io_init(w, &event_loop_ev::read_handler, fd, EV_READ);
    ev_io_start(loop_, w);
  }
  return 0;
  //TODO: clean it

  // return watcher->read_handlers.begin();
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

  socket_queue* sq = reinterpret_cast<socket_queue*>(w->data);

  char tmp_buf[256] = {0};
  auto received_len = sq->socket.receive(tmp_buf, 255);

  auto &handlers = sq->read_handlers;
  auto &cb = handlers.back();

  cb(tmp_buf, received_len);

  handlers.pop();

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
  auto &write_element = handlers.back();

  auto &data = std::get<0>(write_element);
  auto &cb = std::get<1>(write_element);

  sq->socket.send(data);
  cb();

  handlers.pop();

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

void event_loop_ev::stop(event_watcher_t& io)
{
    /* LOG_THIS; */
  //TODO: fill it!
}

}}
