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
  ev_io_init(&readWatcher_, &IOWatcher::ReadHandler, fd, EV_READ);
  ev_io_init(&writeWatcher_, &IOWatcher::WriteHandler, fd, EV_WRITE);

  writeWatcher_.data = this;
  readWatcher_.data = this;
}

IOWatcher::~IOWatcher(){
  loop_.Stop(writeWatcher_);
  loop_.Stop(readWatcher_);
}

void IOWatcher::ReadHandler(EV_P_ ev_io* w, int revents) {
  /* LOG_THIS; */
  if (!(revents & EV_READ)) {
    // LOG_ERR("WRONG EVENT ON read_handler");
    return;
  }

  IOWatcher* sq = reinterpret_cast<IOWatcher*>(w->data);
  auto &handlers = sq->readHandlers_;

  if (handlers.size() != 0)
  {
    auto &action = handlers.front();
    action();
    handlers.pop();
  }

  if (handlers.size() == 0)
    ev_io_stop(loop, &sq->readWatcher_);
}

void IOWatcher::WriteHandler(EV_P_ ev_io* w, int revents)
{
  /* LOG_THIS; */
  if (!(revents & EV_WRITE)) {
    // LOG_ERR("WRONG EVENT ON read_handler");
    return;
  }

  IOWatcher* sq = reinterpret_cast<IOWatcher*>(w->data);
  auto &handlers = sq->writeHandlers_;

  if (handlers.size() != 0)
  {
    auto &action = handlers.front();
    action();
    handlers.pop();
  }

  if (handlers.size() == 0)
    ev_io_stop(loop, &sq->writeWatcher_);
}

}
}
}
