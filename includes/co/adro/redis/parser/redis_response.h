#pragma once

#include "base_resp_parser.h"

#include <memory>

namespace co{
namespace adro{
namespace redis{
namespace parser{
    class redis_response
    {
    public:
      using parser = std::shared_ptr<base_resp_parser>;

      redis_response(parser &ptr);
      int append_chunk(const char* chunk, ssize_t length, bool &is_finished);
      inline bool is_fresh() const;
      inline parser data() const;

    private:
      parser& parser_;
    };
}
}
}
}
