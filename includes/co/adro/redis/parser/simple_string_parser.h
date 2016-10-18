#pragma once

#include "base_resp_parser.h"

namespace co{
namespace adro{
namespace redis{
namespace parser{
    class simple_string_parser : public base_resp_parser
    {
    public:
      simple_string_parser();
      RespType type() const override;
      int parse_append(const char* chunk, ssize_t length, bool& is_finished) override;
      std::string to_string() const override;

    private:
      std::string buffer_;
    };
}
}
}
}
