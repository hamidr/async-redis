#include <co/adro/event_loop/watchers.h>
#include <co/adro/event_loop/event_loop_ev.h>

namespace co{
namespace adro{
namespace event_loop{
TimerWatcher::TimerWatcher(double time, const std::function<void()>& cb)
  : timeoutCB_(cb)
{
  ev_timer_init (&timer_, &TimerWatcher::TimerHandler, time, 0.);
}

void 
TimerWatcher::TimerHandler(EV_P_ ev_timer* w, int revents)
{
  TimerWatcher *watcher = reinterpret_cast<TimerWatcher*>(w);
  std::function<void()>& func(watcher->GetTimeoutCallBack());
  func();
  delete watcher;
}

void 
TimerWatcher::Start(struct ev_loop* loop ){
  ev_timer_start (loop , &timer_);
}

ev_timer&
TimerWatcher::GetWatcher(){
  return timer_; 
}

std::function<void()>&
TimerWatcher::GetTimeoutCallBack(){
  return timeoutCB_; 
}

IOWatcher::IOWatcher(EventLoopEV& loop, int fd)
  : loop_(loop)
{
  ev_io_init(&read_watcher, &IOWatcher::read_handler, fd, EV_READ);
  ev_io_init(&write_watcher, &IOWatcher::write_handler, fd, EV_WRITE);

  write_watcher.data = this;
  read_watcher.data = this;
}

IOWatcher::~IOWatcher(){
  loop_.Stop(write_watcher);
  loop_.Stop(read_watcher);
}

void IOWatcher::read_handler(EV_P_ ev_io* w, int revents) {
  /* LOG_THIS; */
  if (!(revents & EV_READ)) {
    // LOG_ERR("WRONG EVENT ON read_handler");
    return;
  }

  IOWatcher* sq = reinterpret_cast<IOWatcher*>(w->data);
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

void IOWatcher::write_handler(EV_P_ ev_io* w, int revents)
{
  /* LOG_THIS; */
  if (!(revents & EV_WRITE)) {
    // LOG_ERR("WRONG EVENT ON read_handler");
    return;
  }

  IOWatcher* sq = reinterpret_cast<IOWatcher*>(w->data);
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

}
}
}
