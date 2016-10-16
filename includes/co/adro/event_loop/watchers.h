#pragma once 

#include <ev.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <queue>

namespace co{
namespace adro{
namespace event_loop{
    class EventLoopEV;
    class TimerWatcher
    {
      public:
        TimerWatcher(double time, const std::function<void()>& cb);
        static void TimerHandler(EV_P_ ev_timer* w, int revents); 
        ev_timer& GetWatcher(); 
        std::function<void()>& GetTimeoutCallBack(); 
        void Start(struct ev_loop* loop);
      private:
        ev_timer timer_;
        std::function<void()> timeoutCB_;
    };

    class IOWatcher
    {
      public:
        IOWatcher(EventLoopEV& loop, int fd);
        ~IOWatcher();
      public:
        EventLoopEV& loop_;

        ev_io write_watcher;
        ev_io read_watcher;

        static void read_handler(EV_P_ ev_io* w, int revents);
        static void write_handler(EV_P_ ev_io* w, int revents);
        std::queue<std::function<void()> > write_handlers;
        std::queue<std::function<void()> > read_handlers;
    };

}
}
}
