#pragma once

#include <vector>
#include <memory>

#include <co/adro/redis/parser/number_parser.h>
#include <co/adro/redis/parser/bulk_string_parser.h>

namespace co{
namespace adro{
namespace redis{
namespace parser{
    class array_parser : public base_resp_parser
    {
    public:
      RespType type() const override;
      int parse_append(const char* chunk, ssize_t length, bool& is_finished) override;
      string to_string() const override;
      bool IsArray() override {return true;} 
      std::vector<std::shared_ptr<base_resp_parser>> GetArray() override;

    private:
      enum State {
        Size = 0,
        Elems,
        Nil,
        Empty
      };

      std::vector<std::shared_ptr<base_resp_parser>> tree_;

      string size_;
      int size_i_ = 0;
      State state_ = State::Size;
    };
}
}
}
}
