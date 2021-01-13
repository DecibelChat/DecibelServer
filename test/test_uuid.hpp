#include <uuid.hpp>

#include <regex>

#include <catch2/catch.hpp>

TEST_CASE("is valid UUID4", "[UUID]")
{
  websocket_server::uuid::UUID4 uuid_generator;
  const auto generated = uuid_generator();

  const std::regex uuid4_pattern("[a-f0-9]{8}-?[a-f0-9]{4}-?4[a-f0-9]{3}-?[89ab][a-f0-9]{3}-?[a-f0-9]{12}");

  REQUIRE(std::regex_match(generated, uuid4_pattern));
}