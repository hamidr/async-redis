#include "../../includes/parser/number_parser.h"

namespace async_redis {
namespace parser {

RespType number_parser::type() const
{
  return RespType::Num;
}

int number_parser::parse_append(const char* chunk, ssize_t length, bool& is_finished)
{
  int ctr = 0;
  char c = chunk[ctr];

  while(ctr < length)
  {
    switch(c)
    {
    case '\n':
      is_finished = true;
      return ctr + 1;

    case ':':
    case '\r':
      break;

    default:
      buffer_.push_back(c);
      break;
    }

    c = chunk[++ctr];
  }
  return ctr;
}

string number_parser::to_string() const {
  return "Num:"+ buffer_;
}

}}
