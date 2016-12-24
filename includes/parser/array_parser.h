#pragma once

#include <vector>
#include <memory>

#include "number_parser.h"
#include "bulk_string_parser.h"

namespace async_redis {
  namespace parser
  {
    class array_parser : public base_resp_parser
    {
    public:
      RespType type() const override;
      int parse_append(const char* chunk, ssize_t length, bool& is_finished) override;
      string to_string() const override;
      void map(const caller_t& fn) override {
        for(auto &c : tree_)
          fn(*c);
      };

    private:
      enum State {
        Size = 0,
        Elems,
        Nil,
        Empty
      };

      std::vector<std::unique_ptr<base_resp_parser>> tree_;

      string size_;
      int size_i_ = 0;
      State state_ = State::Size;
    };
  }
}
