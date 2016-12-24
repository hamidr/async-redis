#pragma once

#include <string>
#include <functional>

namespace async_redis {
  namespace parser
  {
    using std::string;

    enum class RespType : char
    {
      Err = '-',
      Arr = '*',
      Num = ':',
      Str = '+',
      BulkStr = '$',
    };

    class base_resp_parser
    {
    public:
      using caller_t = std::function<void(const base_resp_parser&)>;

      virtual RespType type() const = 0;
      virtual int parse_append(const char*, ssize_t, bool&) = 0;
      virtual std::string to_string() const = 0;
      virtual void map(const caller_t &fn) {
        fn(*this);
      }
    };
  }
}
