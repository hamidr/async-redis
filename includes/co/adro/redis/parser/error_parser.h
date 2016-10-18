#pragma once

#include <co/adro/redis/parser/base_resp_parser.h>
namespace co{
namespace adro{
namespace redis{
namespace parser{
    class error_parser : public base_resp_parser
    {
    public:
      RespType type() const override;
      int parse_append(const char* chunk, ssize_t length, bool& is_finished) override;
      string to_string() const override;

    private:
      string buffer_;
    };
}
}
}
}
