#include <co/adro/event_loop/event_loop_ev.h>
#include <co/adro/event_loop/watchers.h>

namespace co{
namespace adro{
namespace event_loop {


EventLoopEV::EventLoopEV()
  : loop_(EV_DEFAULT)
{
}

EventLoopEV::EventLoopEV(struct ev_loop* loop)
  : loop_(loop)
{
}

void EventLoopEV::Run()
{
  ev_run (loop_, 0);
}

void EventLoopEV::ASyncWrite(SocketIdentifierT& id, const std::function<void()>& cb)
{
  IOWatcher* watcher = id->second.get();

  auto &handlers = watcher->writeHandlers_;
  handlers.push(cb);

  if (watcher->writeHandlers_.size() == 1) {
    ev_io *w = &watcher->writeWatcher_;
    ev_io_start(loop_, w);
  }
}

void EventLoopEV::ASyncRead(SocketIdentifierT& id, const std::function<void()>& cb)
{
  IOWatcher *watcher = id->second.get();

  auto &handlers = watcher->readHandlers_;
  handlers.push(cb);

  if (watcher->readHandlers_.size() == 1) {
    ev_io *w = &watcher->readWatcher_;
    ev_io_start(loop_, w);
  }
}

void EventLoopEV::ASyncTimeout(double time, const std::function<void()>& cb )
{
  TimerWatcher *w = new TimerWatcher(time, cb);
  w->Start(loop_); 
}


void EventLoopEV::Stop(ev_io& io)
{
  /* LOG_THIS; */
  ev_io_stop(loop_, &io);
}

void EventLoopEV::Start(ev_io& io)
{
  /* LOG_THIS; */
  ev_io_start(loop_, &io);
}

EventLoopEV::SocketIdentifierT EventLoopEV::Watch(int fd)
{
  auto iter = watchers_.find(fd);

  if (iter == watchers_.end()) {
    auto w = watchers_.emplace(fd, std::make_unique<IOWatcher>(*this, fd));
    return w.first;
  }

  return iter;
}

void EventLoopEV::UnWatch(SocketIdentifierT& id)
{
  watchers_.erase(id);
}

}
}
}
