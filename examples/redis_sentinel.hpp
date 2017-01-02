#pragma once

#include <vector>
#include <sentinel.hpp>

namespace async_redis {
  namespace redis_impl
  {
    template<typename InputOutputHandler>
    class redis_sentinel
    {
      using tcp_socket_t = ::async_redis::network::tcp_socket<InputOutputHandler>;

      using sentinel_t = sentinel<InputOutputHandler>;
      using sentinel_ptr_t = std::shared_ptr<sentinel_t>;

    public:
      enum State {
        SentinelsConnectingFailed = 0,
        GetMasterAddrFailed,
        MasterNotFound,
        FoundMaster,
        MasterChanged
      };

      using master_watcher_cb_t = std::function<void (const string& ip, int port, State)>;

      redis_sentinel(InputOutputHandler &io, const string& cluster_name)
        : io_(io), cluster_name_(cluster_name)
      { }

      void connect(const std::vector<std::pair<std::string, int>>& sentinel_addrs, master_watcher_cb_t&& master_watcher_cb)
      {
        for (auto &addr : sentinel_addrs) {
          const string &ip = addr.first;
          const int& port = addr.second;

          string key = ip + ":" + std::to_string(port);
          sentinels_.emplace(key, std::make_unique<sentinel_t>(io_));

          sentinels_[key]->connect(ip, port, std::bind(&redis_sentinel::sentinel_connected, this, key, std::placeholders::_1));
        }

        master_watcher_ = std::make_unique<master_watcher_cb_t>(std::move(master_watcher_cb));
      }

    private:
      void sentinel_connected(const string& key, bool res)
      {
        res ?
          ++sentinels_connected_ :
          ++sentinels_not_connected_;

        if (sentinels_.size() == sentinels_connected_) {
          sentinels_connected_ = 0;
          sentinels_not_connected_ = 0;
          return get_master_addr();
        }

        if ((sentinels_connected_ + sentinels_not_connected_) == sentinels_.size()) {
          sentinels_connected_ = 0;
          sentinels_not_connected_ = 0;

          return call_master_watcher(nullptr, -1, State::SentinelsConnectingFailed);
        }
      }

      void call_master_watcher(const string& ip, int port, State state)
      {
        if (master_watcher_)
          return (*master_watcher_)(ip, port, state);
        //TODO: LOG this
      }

      void get_master_addr()
      {
        // <ip, port, total_sentin, total_got_res>
        auto state = std::make_shared<std::tuple<string, int, int, int>>(std::make_tuple(nullptr, -1, sentinels_.size(), 0));

        auto &sen = sentinels_.begin()->second;
        for(auto &itr : sentinels_)
        {
          auto &sen = itr.second;
          sen->master_addr_by_name(cluster_name_,
            [this, sen, state](const string& ip, int port, bool res)
            {
              string &final_ip = std::get<0>(*state);
              int &final_port = std::get<1>(*state);
              int &num = std::get<2>(*state);
              int &got_res = std::get<3>(*state);

              if (sentinels_.size() == num) {
                //First recieve
                final_port = port;
                final_ip = ip;
              } else {
                // other receives to override the ip and port

                //TODO: what should we do if the sentinels dont agree on ip and port of master
                if (final_port != port || final_ip != ip)
                  /* probably something is wrong! */; 

                final_port = port;
                final_ip = ip;
              }

              if (res)
                ++got_res;
              --num;

              if (num == 0 && got_res > 0) {
                call_master_watcher(ip, port, State::FoundMaster);
                watch_for_master(sen);
              } else if (num == 0 && got_res == 0) {
                call_master_watcher(ip, port, State::MasterNotFound);
              }

            }
          );
        }
      }

      void watch_for_master(const sentinel_ptr_t& sen)
      {
        sen->watch_master_change(
          [this](const string& ip, int port) -> bool
          {
            call_master_watcher(ip, port, State::MasterChanged);
            return true;
          }
        );
      }

    private:
      InputOutputHandler &io_;
      std::unordered_map<string, sentinel_ptr_t> sentinels_;
      std::unique_ptr<master_watcher_cb_t> master_watcher_;
      int sentinels_connected_ = 0;
      int sentinels_not_connected_ = 0;
      const string& cluster_name_;
    };
  }
}
