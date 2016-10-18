#include <co/adro/redis/connection.h>
namespace co{
namespace adro{
namespace redis{
Connection::Connection(event_loop::EventLoopEV &eventLoop, network::AsyncSocket* socket)
  : eventLoop_(eventLoop), socket_(socket) {
}

template<typename ...Args>
inline void 
Connection::Connect(Args... args) {
  socket_->template AsyncConnect<network::AsyncSocket>(0, std::forward<Args>(args)...);
}

bool 
Connection::IsConnected() const
{ return socket_ && socket_->IsConnected(); }

inline int 
Connection::Pressure() const {
  return reqQueue_.size();
}

void 
Connection::Send(const std::string& command, const std::function<void(std::shared_ptr<parser::base_resp_parser>)>& reply_cb) {

  socket_->AsyncWrite(command, [this, reply_cb]() {
      reqQueue_.emplace(reply_cb, nullptr);

      if (reqQueue_.size() == 1)
        socket_->AsyncRead(// buffer_, max_length, 
                            std::bind(&Connection::ReplyReceived, this, std::placeholders::_1, std::placeholders::_2));
    });
}

void 
Connection::ReplyReceived(const char* data, int len) {
  ssize_t acc = 0;

  while (acc < len && reqQueue_.size()) {
    auto& request = reqQueue_.front();

    auto &cb = std::get<0>(request);
    auto &parser = std::get<1>(request);

    if (0 != len && -1 != len) {

      bool is_finished = false;
      acc += parser::redis_response(parser).append_chunk(data + acc, len - acc, is_finished);

      if (!is_finished)
        break;

      cb(parser);
      reqQueue_.pop(); //free the resources

    }
  }

  if (reqQueue_.size() != 0)
    socket_->AsyncRead(// buffer_, max_length, 
                        std::bind(&Connection::ReplyReceived, this, std::placeholders::_1, std::placeholders::_2));
}
}
}
}
