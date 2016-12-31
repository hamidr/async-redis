#include "../../includes/parser/array_parser.h"

namespace async_redis {
namespace parser {

RespType array_parser::type() const
{
  return RespType::Arr;
}

int array_parser::parse_append(const char* chunk, ssize_t length, bool& is_finished)
{
  ssize_t ctr = 0;
  char c = chunk[ctr];
  bool finished = false;

  while(ctr < length)
  {
    switch(c)
    {
    case '*':
      switch(state_)
      {
      case State::Elems:
        tree_.push_back(std::make_shared<array_parser>());
        break;
      }
      break;

    case ':':
      tree_.push_back(std::make_shared<number_parser>());
      break;

    case '$':
      tree_.push_back(std::make_shared<bulk_string_parser>());
      break;

    case '\r':
      break;

    case '\n':
      switch(state_)
      {
      case State::Size:
        size_i_ = std::stoi(size_);

        switch(size_i_)
        {
        case -1:
          is_finished = true;
          state_ = State::Nil;
          return ctr + 1;

        case 0:
          is_finished = true;
          state_ = State::Empty;
          return ctr + 1;
        }

        tree_.reserve(size_i_);
        state_ = State::Elems;
        break;

      case State::Elems:
        finished = false;
        ctr += tree_.back()->parse_append(chunk + ctr, length - ctr, finished);

        if (finished) {
          --ctr;
          --size_i_;
        }

        if (0 == size_i_) {
          is_finished = true;
          return ctr + 1;
        }
        break;
      }
      break;

    default:
      switch(state_)
      {
      case State::Size:
        size_.push_back(c);
        break;

      case State::Elems:
        finished = false;
        ctr += tree_.back()->parse_append(chunk + ctr, length - ctr, finished);

        if (finished) {
          --ctr;
          --size_i_;
        }

        if (0 == size_i_) {
          is_finished = true;
          return ctr + 1;
        }
        break;
      }
    }

    c = chunk[++ctr];
  }
  return ctr;
}

string array_parser::to_string() const
{
  string s = "[";

  for (auto &leaf : tree_)
    s += (leaf->to_string() + " ");

  return s + "]";
}

}}
