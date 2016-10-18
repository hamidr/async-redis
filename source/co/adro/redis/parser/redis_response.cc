#include <co/adro/redis/parser/redis_response.h>

#include <co/adro/redis/parser/number_parser.h>
#include <co/adro/redis/parser/error_parser.h>
#include <co/adro/redis/parser/bulk_string_parser.h>
#include <co/adro/redis/parser/array_parser.h>
#include <co/adro/redis/parser/simple_string_parser.h>

namespace co{
namespace adro{
namespace redis{
namespace parser{
redis_response::redis_response(parser &ptr)
  : parser_(ptr)
{
}

int redis_response::append_chunk(const char* chunk, ssize_t length, bool &is_finished) {
  /* LOG_THIS */
  if (is_fresh()) {
    switch(static_cast<RespType>(chunk[0]))
    {
    case RespType::Err:
      parser_ = std::make_shared<error_parser>();
      break;

    case RespType::Arr:
      parser_ = std::make_shared<array_parser>();
      break;

    case RespType::Num:
      parser_ = std::make_shared<number_parser>();
      break;

    case RespType::Str:
      parser_ = std::make_shared<simple_string_parser>();
      break;

    case RespType::BulkStr:
      parser_ = std::make_shared<bulk_string_parser>();
      break;

    default:
      /* std::cerr << "Parsed not initialized! MUST NOT Happen!" << std::endl; */
      //Todo: Exception!
      return 0;
    }
  }

  return parser_->parse_append(chunk, length, is_finished);
}

inline bool redis_response::is_fresh() const
{
  return !parser_;
}

inline redis_response::parser redis_response::data() const
{
  return parser_;
}

}
}
}
}
