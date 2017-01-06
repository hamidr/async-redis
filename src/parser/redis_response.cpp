#include "../../includes/parser/redis_response.h"

#include "../../includes/parser/number_parser.h"
#include "../../includes/parser/error_parser.h"
#include "../../includes/parser/bulk_string_parser.h"
#include "../../includes/parser/array_parser.h"
#include "../../includes/parser/simple_string_parser.h"

namespace async_redis {
namespace parser {


int
redis_response::append_chunk(parser& data, const char* chunk, ssize_t length, bool &is_finished)
{
  if (data)
    goto handle;

  switch(static_cast<RespType>(chunk[0]))
  {
  case RespType::Err:
    data = std::make_shared<error_parser>();
    break;

  case RespType::Arr:
    data = std::make_shared<array_parser>();
    break;

  case RespType::Num:
    data = std::make_shared<number_parser>();
    break;

  case RespType::Str:
    data = std::make_shared<simple_string_parser>();
    break;

  case RespType::BulkStr:
    data = std::make_shared<bulk_string_parser>();
    break;

  default:
    /* std::cerr << "Parsed not initialized! MUST NOT Happen!" << std::endl; */
    //Todo: Exception!
    return 0;
  }

 handle:
  return data->parse_append(chunk, length, is_finished);
}

}}
