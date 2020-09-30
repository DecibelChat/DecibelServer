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
    DELETE
  };

  const std::unordered_map<MessageType, std::string> message_type_to_string = {{MessageType::SDP, "SDP"},
                                                                               {MessageType::CANDIDATE, "CANDIDATE"},
                                                                               {MessageType::SERVER, "SERVER"},
                                                                               {MessageType::DELETE, "DELETE"}};

  auto generate_reverse_dict = [](auto dict) {
    using input_type = decltype(dict);
    std::unordered_map<typename input_type::mapped_type, typename input_type::key_type> inverse_dict;

    for (auto [k, v] : dict)
    {
      inverse_dict[v] = k;
    }

    return inverse_dict;
  };

  const auto string_to_message_type = generate_reverse_dict(message_type_to_string);

} // namespace websocket_server