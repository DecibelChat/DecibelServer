#include <ClientInfo.h>

#include <catch2/catch.hpp>

TEST_CASE("room assignment", "[ClientInfo]")
{
  websocket_server::ClientInfo client_info;

  SECTION("room is initially unassigned")
  {
    REQUIRE(client_info.unassigned());
  }
}

TEST_CASE("distance", "[ClientInfo]")
{
  websocket_server::ClientInfo client_info;

  SECTION("update position returns true when changed")
  {
    // set to 0, because we're not testing the initialized value.
    client_info.update_position({0, 0, 0});

    REQUIRE(false == client_info.update_position({0, 0, 0}));
    REQUIRE(true == client_info.update_position({1, 0, 0}));
    REQUIRE(false == client_info.update_position({1, 0, 0}));
    REQUIRE(true == client_info.update_position({0, 0, 0}));
  }

  SECTION("distance to self is 0")
  {
    REQUIRE(0.f == client_info.distance(client_info));
  }

  SECTION("distance computed is accurate")
  {
    client_info.update_position({0, 0, 0});

    websocket_server::ClientInfo info_2;
    info_2.update_position({10, 0, 0});

    REQUIRE(10 == client_info.distance(info_2));
    REQUIRE(10 == info_2.distance(client_info));

    info_2.update_position({10, 10, 10});
    REQUIRE(Approx(std::sqrt(300)) == client_info.distance(info_2));
    REQUIRE(Approx(std::sqrt(300)) == info_2.distance(client_info));
  }
}