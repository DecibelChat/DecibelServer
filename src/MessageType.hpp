#pragma once

#include <string>
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

  const std::unordered_map<MessageType, std::string> message_type_to_string = {{MessageType::SDP, "SDP"},
                                                                               {MessageType::CANDIDATE, "CANDIDATE"},
                                                                               {MessageType::SERVER, "SERVER"},
                                                                               {MessageType::VOLUME, "VOLUME"},
                                                                               {MessageType::POSITION, "POSITION"}};

  auto generate_reverse_dict = [](auto dict) {
    using input_type = typename std::remove_const<typename std::remove_reference<decltype(dict)>::type>::type;
    std::unordered_map<typename input_type::mapped_type, typename input_type::key_type> inverse_dict;

    for (auto [k, v] : dict)
    {
      inverse_dict[v] = k;
    }

    return inverse_dict;
  };

  const auto string_to_message_type = generate_reverse_dict(message_type_to_string);

  template <typename BasicJsonType>
  inline void to_json(BasicJsonType &j, const MessageType &message)
  {
    j = message_type_to_string.at(message);
  }
  template <typename BasicJsonType>
  inline void from_json(const BasicJsonType &j, MessageType &message)
  {
    std::string s = j;
    message       = string_to_message_type.at(s);
  }

} // namespace websocket_server