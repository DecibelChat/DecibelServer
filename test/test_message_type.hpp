#include <MessageType.hpp>

#include <catch2/catch.hpp>
#include <nlohmann/json.hpp>

TEST_CASE("serialize/deserialize MessageType", "[MessageType]")
{
  using namespace websocket_server;

  constexpr auto message_value  = MessageType::SERVER;
  constexpr auto message_string = "SERVER";

  SECTION("to string")
  {
    REQUIRE(message_string == to_string(message_value));
  }

  SECTION("from string")
  {
    REQUIRE(message_value == from_string(message_string));
  }

  SECTION("to json")
  {
    REQUIRE_NOTHROW(nlohmann::json{message_value});
  }

  SECTION("from json")
  {
    nlohmann::json j = message_value;
    REQUIRE_NOTHROW(j.get<MessageType>());
    REQUIRE(message_value == j.get<MessageType>());
    REQUIRE(message_string == j.get<std::string>());
  }
}