#include "../../includes/parser/bulk_string_parser.h"


namespace async_redis {
namespace parser {

RespType bulk_string_parser::type() const {
  return RespType::BulkStr;
}

int bulk_string_parser::parse_append(const char* chunk, ssize_t length, bool& is_finished)
{
  int ctr = 0;
  char c = chunk[ctr];
  while(ctr < length)
  {
    switch(c)
    {
    case '$':
    case '\r':
      break;

    case '\n':
      switch(state_)
      {
      case State::Number:
        size_ = std::stoi(num_);
        if (-1 == size_) {
          is_finished = true;
          state_ = State::Nil;
          return ctr + 1;
        }

        buffer_.reserve(size_);
        state_ = State::String;
        break;

      case State::String:
        is_finished = true;
        return ctr + 1;
        break;
      }
      break;

    default:
      switch(state_)
      {
      case State::Number:
        num_.push_back(c);
        break;
      case State::String:
        buffer_.push_back(c);
        break;
      }
    }
    c = chunk[++ctr];
  }

  return ctr;
}

string bulk_string_parser::to_string() const
{
  return "BulkStr:"+buffer_;
}

}}
