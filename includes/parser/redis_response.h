#pragma once

#include "base_resp_parser.h"

#include <memory>

namespace async_redis {
  namespace parser
  {
    class redis_response
    {
    public:
      using parser = std::shared_ptr<base_resp_parser>;

      static int append_chunk(parser &, const char*, ssize_t, bool &);
    };
  }
}
