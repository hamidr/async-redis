#include "../includes/async_redis/sentinel.hpp"

#include <istream>
#include <sstream>

namespace async_redis
{
sentinel::sentinel(event_loop::event_loop_ev &event_loop)
  : conn_(event_loop),
    stream_(event_loop)
{ }

bool sentinel::is_connected() const
{
  return stream_.is_connected() && conn_.is_connected();
}

bool sentinel::connect(const string& ip, int port, connect_cb_t&& connector)
{
  if (is_connected())
    return false;

  connect_all(ip, port, connector);
  return true;
}

void sentinel::disconnect() {
  stream_.disconnect();
  conn_.disconnect();
}

bool sentinel::failover(const string& clustername, connection::reply_cb_t&& reply)
{
  return if_connected_do(
    [&]() -> bool {
      return send({std::string("sentinel failover ") + clustername}, std::move(reply));
    }
  );
}

bool sentinel::ping(connection::reply_cb_t&& reply)
{
  return if_connected_do(
    [&]() -> bool {
      return send({"ping"}, std::move(reply));
    }
  );
}

bool sentinel::watch_master_change(cb_watch_master_change_t&& fn)
{
  return if_connected_do(
    [&]()-> bool {
      using State = monitor::EventState;

      return stream_.subscribe({"+switch-master"},
        [this, fn = std::move(fn)](const string& channel, parser_t event, State state) -> void
        {
          switch(state)
          {
          case State::Stream:
            return fn(parse_watch_master_change(event), SentinelState::Watching);

          case State::Disconnected:
            this->disconnect();
            return fn({}, SentinelState::Disconnected);
            break;

          default:
            break;
          }
        }
      );
    }
  );
}


bool sentinel::master_addr_by_name(const string& cluster_name, cb_addr_by_name_t&& cb)
{
  return if_connected_do(
    [&]() -> bool {
      return send_master_addr_by_name(cluster_name, std::move(cb));
    }
  );
}

bool sentinel::send_master_addr_by_name(const string& cluster_name, cb_addr_by_name_t&& cb)
{
  return this->send({string("SENTINEL get-master-addr-by-name ") + cluster_name + "\r\n"},
    [cb = std::move(cb)](parser_t parsed_value)
    {
      using ::async_redis::parser::RespType;

      if (parsed_value->type() == RespType::Err)
        return cb(nullptr, -1, false);

      int elems = 0;

      string addr;
      int port = -1;

      parsed_value->map(
        [&elems, &addr, &port](const parser::base_resp_parser& val)
        {
          switch(elems)
          {
          case 0:
            addr = val.to_string();
            break;
          case 1:
            port = std::stoi(val.to_string());
            break;
          }

          ++elems;
        }
      );

      cb(addr, port, elems != 2);
    }
  );
}

// static
std::vector<std::string> sentinel::parse_watch_master_change(const parser_t& event)
{
  std::vector<string> words;
  std::istringstream iss(event->to_string());

  std::string s;
  while ( getline( iss, s, ' ' ) )
    words.push_back(s);

  return words;
}

bool sentinel::if_connected_do(std::function<bool ()>&& fn)
{
  if (!is_connected())
    return false;

  return fn();
}

void sentinel::connect_all(const string& ip, int port, const connect_cb_t& connector)
{
  auto cb = std::bind(&sentinel::check_connected, this, connector, std::placeholders::_1);

  conn_.connect(cb, ip, port);
  stream_.connect(cb, ip, port);
}

void sentinel::check_connected(const connect_cb_t& connector, bool res)
{
  if (++connected_ != 2)
    return;

  if (!is_connected())
    disconnect();

  connected_ = 0;
  connector(is_connected());
}

bool sentinel::send(std::list<string>&& words, connection::reply_cb_t&& reply)
{
  string cmd = words.front();
  words.pop_front();
  for(auto &w : words)
    cmd += " " + w;
  cmd += "\r\n";

  if (!conn_.send(std::move(cmd), std::move(reply))) {
    disconnect();
    return false;
  }

  return true;
}

}
