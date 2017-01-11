#include "../includes/async_redis/monitor.hpp"

#include <async_redis/parser/array_parser.h>
#include <cassert>
#include <libevpp/network/tcp_socket.hpp>
#include <libevpp/network/unix_socket.hpp>

namespace async_redis
{
using tcp_socket      = network::tcp_socket;
using unix_socket     = network::unix_socket;

monitor::monitor(event_loop::event_loop_ev &event_loop)
  : io_(event_loop)
{}

void monitor::connect(async_socket::connect_handler_t handler, const std::string& ip, int port)
{
  if (!socket_ || !socket_->is_valid())
    socket_ = std::make_unique<tcp_socket>(io_);

  static_cast<tcp_socket*>(socket_.get())->async_connect(ip, port, handler);
}

void monitor::connect(async_socket::connect_handler_t handler, const std::string& path)
{
  if (!socket_ || !socket_->is_valid())
    socket_ = std::make_unique<unix_socket>(io_);

  static_cast<unix_socket*>(socket_.get())->async_connect(path, handler);
}

bool monitor::is_connected() const
{ return socket_->is_connected(); }

bool monitor::is_watching() const
{ return this->is_connected() && is_watching_; }

void monitor::disconnect()
{
  socket_->close();

  pwatchers_.clear();
  watchers_.clear();
  is_watching_ = false;
}

bool monitor::psubscribe(const std::list<string>& channels, watcher_cb_t&& cb)
{
  assert(channels.size());
  string start_cmd = "psubscribe";

  for(auto &ch : channels) {
    start_cmd += " " + ch;
    pwatchers_.emplace(ch, cb);
  }

  start_cmd += "\r\n";
  return send_and_receive(std::move(start_cmd));
}

bool monitor::subscribe(const std::list<string>& channels, watcher_cb_t&& cb)
{
  assert(channels.size());

  string cmd = "subscribe";

  for(auto &ch : channels) {
    cmd += " " + ch;
    watchers_.emplace(ch, cb);
  }

  cmd += "\r\n";
  return send_and_receive(std::move(cmd));
}

bool monitor::unsubscribe(const std::list<string>& channels, watcher_cb_t&& cb)
{
  assert(channels.size());

  string cmd = "unsubscribe";
  for(auto &ch : channels)
    cmd += " " + ch;
  cmd += "\r\n";

  return send_and_receive(std::move(cmd));
}

bool monitor::punsubscribe(const std::list<string>& channels, watcher_cb_t&& cb)
{
  assert(channels.size());

  string cmd = "punsubscribe";
  for(auto &ch : channels)
    cmd += " " + ch;
  cmd += "\r\n";

  return send_and_receive(std::move(cmd));
}

bool monitor::send_and_receive(string&& data)
{
  if (!is_connected())
    return false;

  socket_->async_write(data, [this](ssize_t sent_chunk_len) {
    if (is_watching_)
      return;

    this->socket_->async_read(
      this->data_,
      this->max_data_size,
      std::bind(
        &monitor::stream_received,
        this,
        std::placeholders::_1
      )
    );

    this->is_watching_ = true;
  });
  return true;
}

void monitor::handle_message_event(parser_t& channel, parser_t& value)
{
  const string& ch_key = channel->to_string();
  auto itr = watchers_.find(ch_key);
  assert(itr != watchers_.end());
  itr->second(ch_key, value, EventState::Stream);
}

void monitor::handle_subscribe_event(parser_t& channel, parser_t& clients)
{
  const string& ch_key = channel->to_string();
  auto itr = watchers_.find(ch_key);
  assert(itr != watchers_.end());
  itr->second(ch_key, clients, EventState::Sub);
}

void monitor::handle_psubscribe_event(parser_t& channel, parser_t& clients)
{
  const string& ch_key = channel->to_string();
  auto itr = pwatchers_.find(ch_key);
  assert(itr != pwatchers_.end());
  itr->second(ch_key, clients, EventState::Sub);
}

void monitor::handle_punsubscribe_event(parser_t& pattern, parser_t& clients)
{
  auto p_key = pattern->to_string();
  auto itr = pwatchers_.find(p_key);
  assert(itr != pwatchers_.end());
  itr->second(p_key, clients, EventState::Unsub);
  pwatchers_.erase(itr);
}

void monitor::handle_unsubscribe_event(parser_t& channel, parser_t& clients)
{
  auto ch_key = channel->to_string();
  auto itr = watchers_.find(ch_key);
  assert(itr != watchers_.end());
  itr->second(ch_key, clients, EventState::Unsub);
  watchers_.erase(itr);
}

void monitor::handle_pmessage_event(parser_t& pattern, parser_t& channel, parser_t& value)
{
  auto itr = pwatchers_.find(pattern->to_string());
  assert(itr != pwatchers_.end());
  itr->second(channel->to_string(), value, EventState::Stream);
}

void monitor::handle_event(parser_t&& request)
{
  assert(request->type() == async_redis::parser::RespType::Arr);

  auto& event = static_cast<async_redis::parser::array_parser&>(*request);

  assert(event.size() >= 3);

  string type = event.nth(0)->to_string();

  if (type == "message")
    return handle_message_event(event.nth(1), event.nth(2));
  else if (type == "pmessage")
    return handle_pmessage_event(event.nth(1), event.nth(2), event.nth(3));
  else if (type == "subscribe")
    return handle_subscribe_event(event.nth(1), event.nth(2));
  else if (type == "unsubscribe")
    return handle_unsubscribe_event(event.nth(1), event.nth(2));
  else if (type == "psubscribe")
    return handle_psubscribe_event(event.nth(1), event.nth(2));
  else if (type == "punsubscribe")
    return handle_punsubscribe_event(event.nth(1), event.nth(2));

  assert(false);
}

void monitor::report_disconnect()
{
  //Swap it! cause if we call this->disconnect inside the functors
  //then it will be freeing the stackframes of functions!
  decltype(watchers_) t1, t2;
  t1.swap(watchers_);
  t2.swap(pwatchers_);

  string str;

  for(auto &w : t1)
    w.second(str, nullptr, EventState::Disconnected);

  for(auto &w : t2)
    w.second(str, nullptr, EventState::Disconnected);

  disconnect();
}

void monitor::stream_received(ssize_t len)
{
  if (len == 0)
    return report_disconnect();

  ssize_t acc = 0;
  while (acc < len)
  {
    bool is_finished = false;
    acc += parser::base_resp_parser::append_chunk(parser_, data_ + acc, len - acc, is_finished);

    if (!is_finished)
      break;

    { // pass the parser and be done with it
      parser_t event;
      std::swap(event, parser_);

      handle_event(std::move(event));
    }
  }

  // if (!(watchers_.size() || pwatchers_.size()))
  if (!watchers_.size() && !pwatchers_.size()) {
    is_watching_ = false;
    return;
  }

  this->socket_->async_read(
    this->data_,
    this->max_data_size,
    std::bind(
      &monitor::stream_received,
      this,
      std::placeholders::_1
    )
  );
}

}
