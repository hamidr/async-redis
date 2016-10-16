#pragma once

#include <ev.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <queue>

namespace co {
namespace adro{
namespace event_loop{
  class IOWatcher;
  class TimerWatcher;
  class EventLoopEV
  {
    public:
      using SocketIdentifierT = std::unordered_map<int, std::unique_ptr<IOWatcher>>::iterator;
    public:
      EventLoopEV();
      EventLoopEV(struct ev_loop *);

      void Run();

      SocketIdentifierT Watch(int);
      void UnWatch(SocketIdentifierT&);

      void ASyncWrite(SocketIdentifierT& id, const std::function<void()>& cb);
      void ASyncRead(SocketIdentifierT& id, const std::function<void()>& cb);
      void ASyncTimeout(double time, const std::function<void()>& cb );
      void Stop(ev_io&);
      void Start(ev_io&);
    private:
      struct ev_loop* loop_;
      std::unordered_map<int, std::unique_ptr<IOWatcher>> watchers_;
    };
}
}
}
