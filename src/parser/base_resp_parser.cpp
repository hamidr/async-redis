#include "../../includes/async_redis/parser/base_resp_parser.h"

#include "../../includes/async_redis/parser/number_parser.h"
#include "../../includes/async_redis/parser/bulk_string_parser.h"
#include "../../includes/async_redis/parser/array_parser.h"
#include "../../includes/async_redis/parser/error_parser.h"
#include "../../includes/async_redis/parser/simple_string_parser.h"


#include <iostream>

namespace async_redis {
namespace parser {


int
base_resp_parser::append_chunk(base_resp_parser::parser& data, const char* chunk, ssize_t length, bool &is_finished)
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

bool base_resp_parser::is_array() const noexcept
{ return this->type() == RespType::Arr; }

bool base_resp_parser::is_number() const noexcept
{ return this->type() == RespType::Num; }

bool base_resp_parser::is_error() const noexcept
{ return this->type() == RespType::Err; }

bool base_resp_parser::is_string() const noexcept
{ return this->type() == RespType::BulkStr; }

bool base_resp_parser::is_enum() const noexcept
{ return this->type() == RespType::Str; }

void
base_resp_parser::print()
{
  string type;
  switch(this->type()) {
  case RespType::BulkStr:
    type = "bulkstr";
    break;
  case RespType::Arr:
    type = "array";
    break;
  case RespType::Str:
    type = "str";
    break;
  case RespType::Num:
    type = "num";
    break;
  default:
    type = "err";
    break;
  }
  std::cout << "Type: " << type << " Value: " << to_string() << std::endl;
}

void
base_resp_parser::map(const base_resp_parser::caller_t &fn)
{
  fn(*this);
}

}}
