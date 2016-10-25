#pragma once
#include <memory>
#include <vector>
#include <string>

namespace co{
namespace adro{
namespace redis{
namespace parser{
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
      virtual RespType type() const = 0;
      virtual int parse_append(const char*, ssize_t, bool&) = 0;
      virtual std::string to_string() const = 0;
      virtual bool IsArray(){return false;}
      virtual std::vector<std::shared_ptr<base_resp_parser>> GetArray() { return std::vector<std::shared_ptr<base_resp_parser> >(); }
    };
}
}
}
}
