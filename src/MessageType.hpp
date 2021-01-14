#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace websocket_server
{
  enum class MessageType
  {
    SDP,
    CANDIDATE,
    SERVER,
    VOLUME,
    POSITION
  };

  std::string to_string(const MessageType &);
  MessageType from_string(std::string_view);

  template <typename BasicJsonType>
  inline void to_json(BasicJsonType &j, const MessageType &message)
  {
    j = to_string(message);
  }
  template <typename BasicJsonType>
  inline void from_json(const BasicJsonType &j, MessageType &message)
  {
    message = from_string(j);
  }

} // namespace websocket_server