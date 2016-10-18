#include <co/adro/redis/parser/simple_string_parser.h>

namespace co{
namespace adro{
namespace redis{
namespace parser {

simple_string_parser::simple_string_parser()
{
  buffer_.reserve(3);
}

RespType simple_string_parser::type() const
{
  return RespType::Str;
}

int simple_string_parser::parse_append(const char* chunk, ssize_t length, bool& is_finished)
{
  int ctr = 0;
  char c = chunk[ctr];

  while(ctr < length)
  {
    switch(c)
    {
    case '+':
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

std::string simple_string_parser::to_string() const
{
  return "Str:"+buffer_;
}

}
}
}
}
