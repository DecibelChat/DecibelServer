#include <MessageType.hpp>

#include <fmt/format.h>
#include <magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace websocket_server
{
  std::string to_string(const MessageType &value)
  {
    return std::string{magic_enum::enum_name(value)};
  }

  MessageType from_string(std::string_view view)
  {
    try
    {
      return *magic_enum::enum_cast<MessageType>(view);
    }
    catch (const std::bad_optional_access &e)
    {
      throw std::out_of_range(fmt::format("No websocket_server::MessageType value for {}", view));
    }
  }
} // namespace websocket_server