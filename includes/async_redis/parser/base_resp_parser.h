#pragma once

#include <string>
#include <functional>
#include <memory>

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
      using parser = std::shared_ptr<base_resp_parser>;

      static int append_chunk(parser &, const char*, ssize_t, bool &);

      virtual RespType type() const = 0;
      virtual int parse_append(const char*, ssize_t, bool&) = 0;
      virtual std::string to_string() const = 0;
      virtual void map(const caller_t &fn);
      bool is_array() const;
      bool is_number() const;
      bool is_string() const;
      bool is_enum() const;
      bool is_error() const;

      void print();
    };
  }
}
