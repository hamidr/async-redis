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
      using sentinel_ptr_t = std::unique_ptr<sentinel_t>;

    public:
      enum FailedState {
        SentinelsConnecting = 0,
        GetMasterAddr,
        ConnectMaster,
        WatchMasterChange,
      };

      using redis_client_t = redis_client<InputOutputHandler, tcp_socket_t>;
      using redis_client_ptr_t = std::shared_ptr<redis_client_t>;
      using master_watcher_cb_t = std::function<void (redis_client_ptr_t , FailedState)>;

      redis_sentinel(InputOutputHandler &io)
        : io_(io)
      {
      }

      void connect(const std::vector<std::pair<std::string, int>>& sentinel_addrs, master_watcher_cb_t&& master_watcher_cb)
      {
        for (auto &addr : sentinel_addrs)
        {
          const string &ip = addr.first;
          const int& port = addr.second;

          string key = ip + ":" + std::to_string(port);
          sentinels_.emplace(key, std::make_unique<sentinel_t>(io_));

          sentinels_[key]->connect(ip, port, std::bind(&redis_sentinel::sentinel_connected, this, key, std::placeholders::_1));
        }

        master_watcher_ = std::make_unique<master_watcher_cb_t>(std::move(master_watcher_cb));
      }


    private:
      void sentinel_connected(const string& key, bool res) {
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

          return call_master_watcher(nullptr, FailedState::SentinelsConnecting);
        }
      }

      void call_master_watcher(redis_client_ptr_t client, FailedState state)
      {
        if (master_watcher_)
          return (*master_watcher_)(client, state);
      }

      void get_master_addr()
      {
        // master_watcher_(nullptr, )
      }


    private:
      InputOutputHandler &io_;
      std::unordered_map<string, sentinel_ptr_t> sentinels_;
      redis_client_ptr_t master_;
      int sentinels_connected_ = 0;
      int sentinels_not_connected_ = 0;
      std::unique_ptr<master_watcher_cb_t> master_watcher_;

    };

  }
}
