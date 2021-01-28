#include <uuid.hpp>

#include <catch2/catch.hpp>

#include <array>
#include <regex>

TEST_CASE("is valid UUID4", "[UUID]")
{
  websocket_server::uuid::UUID4 uuid_generator;
  const auto generated = uuid_generator();

  constexpr std::array<std::string_view, 5> uuid_version_patterns = {
      /*v1*/ "[0-9A-F]{8}-[0-9A-F]{4}-[1][0-9A-F]{3}-[89AB][0-9A-F]{3}-[0-9A-F]{12}",
      /*v2*/ "[0-9A-F]{8}-[0-9A-F]{4}-[2][0-9A-F]{3}-[89AB][0-9A-F]{3}-[0-9A-F]{12}",
      /*v3*/ "[0-9A-F]{8}-[0-9A-F]{4}-[3][0-9A-F]{3}-[89AB][0-9A-F]{3}-[0-9A-F]{12}",
      /*v4*/ "[a-f0-9]{8}-?[a-f0-9]{4}-?4[a-f0-9]{3}-?[89ab][a-f0-9]{3}-?[a-f0-9]{12}",
      /*v5*/ "[0-9A-F]{8}-[0-9A-F]{4}-[5][0-9A-F]{3}-[89AB][0-9A-F]{3}-[0-9A-F]{12}"};

  constexpr auto desired_version = 4;

  const std::regex uuid4_pattern(uuid_version_patterns.at(desired_version - 1).data());

  REQUIRE(std::regex_match(generated, uuid4_pattern));
}