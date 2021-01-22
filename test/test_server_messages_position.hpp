#include "mock_server.hpp"

TEST_CASE("send/receive MessageType::POSITION", "[Server,POSITION]")
{
  using namespace websocket_server;
  using namespace websocket_server::mock;

  const std::string key_file_name  = "";
  const std::string cert_file_name = "";
  const Parameters parameters      = {
      16666, cert_file_name, key_file_name, 0, 0, fs::temp_directory_path() / "decibel_server" / "test.log"};

  MockServer server(parameters);
  server.start();

  std::unordered_map<std::string, std::shared_ptr<MockClient>> clients;
  auto add_client = [&clients](auto &server, std::string room_id = "test room") {
    auto client = std::make_shared<MockClient>(server);
    nlohmann::json j;
    j["message_type"] = MessageType::CANDIDATE;
    j["code"]         = "test room";
    auto responses    = client->send(j);

    REQUIRE(responses.size() == 1);
    auto &response = responses.front();
    REQUIRE(response["message_type"].get<MessageType>() == MessageType::SERVER);
    REQUIRE(response.contains("content"));
    REQUIRE(response["content"].get<std::string>() == "your id");
    REQUIRE(response["peer_id"].get<std::string>() == client->id());

    clients[client->id()] = client;
    // clients.emplace(client.id(), client);
  };

  add_client(server);

  SECTION("set and get own position")
  {
    REQUIRE(clients.size() == 1);
    auto [id, client] = *clients.begin();

    nlohmann::json j;
    j["message_type"]        = MessageType::POSITION;
    j["code"]                = "test room";
    j["content"]["position"] = {{"x", 1.0, "y", 2.0, "z", 3.0}};
    auto responses           = client->send(j);
    REQUIRE(responses.size() == 0);

    nlohmann::json jj;
    jj["message_type"]       = MessageType::POSITION;
    jj["code"]               = "test room";
    jj["content"]["peer_id"] = client->id(); //    = {{"peer_id", client->id()}};
    responses                = client->send(jj);

    REQUIRE(responses.size() == 1);
    auto &response = responses.front();
    REQUIRE(response["message_type"].get<MessageType>() == MessageType::POSITION);
    REQUIRE(response["content"][client->id()].get<float>() == Approx(0));
  }
}
