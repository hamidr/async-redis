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

        static void ReadHandler(EV_P_ ev_io* w, int revents);
        static void WriteHandler(EV_P_ ev_io* w, int revents);
      public:
        EventLoopEV& loop_;

        ev_io writeWatcher_;
        ev_io readWatcher_;

        std::queue<std::function<void()> > writeHandlers_;
        std::queue<std::function<void()> > readHandlers_;
    };

}
}
}
