#pragma once

#include "base_resp_parser.h"

namespace async_redis {
  namespace parser
  {
    class bulk_string_parser : public base_resp_parser
    {
    public:
      RespType type() const override;
      int parse_append(const char* chunk, ssize_t length, bool& is_finished) override;
      string to_string() const override;

    private:
      enum State {
        Number = 0,
        String,
        Nil
      };

      string buffer_;
      string num_;
      int size_  = 0;

      State state_ = State::Number;
    };
  }
}
