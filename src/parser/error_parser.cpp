#include "../../includes/parser/error_parser.h"

namespace async_redis {
namespace parser {

RespType error_parser::type() const {
  return RespType::Err;
}

int error_parser::parse_append(const char* chunk, ssize_t length, bool& is_finished)
{
  int ctr = 0;
  char c = chunk[ctr];

  while(ctr < length)
  {
      switch(c)
        {
        case ':':
        case '\r':
          break;

        case '\n':
          is_finished = true;
          return ctr + 1;

        default:
          buffer_.push_back(c);
          break;
        }

      c = chunk[++ctr];
    }

  return ctr;
}

string error_parser::to_string() const {
  return buffer_;
}

}}
