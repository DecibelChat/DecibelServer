#include "mock_server.hpp"

#include <cmath>
#include <tuple>
#include <unordered_map>
#include <vector>

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
    auto responses    = client->send(nlohmann::json({{"message_type", MessageType::CANDIDATE}, {"code", "test room"}}));

    REQUIRE(responses.size() == 1);
    auto &response = responses.front();
    REQUIRE(response ==
            nlohmann::json({{"message_type", MessageType::SERVER}, {"content", "your id"}, {"peer_id", client->id()}}));

    clients[client->id()] = client;
  };

  add_client(server);

  SECTION("set and get own position")
  {
    REQUIRE(clients.size() == 1);
    auto [id, client] = *clients.begin();

    nlohmann::json j;
    j["message_type"]        = MessageType::POSITION;
    j["code"]                = "test room";
    j["content"]["position"] = {{"x", 1.0}, {"y", 2.0}, {"z", 3.0}};
    auto responses           = client->send(j);
    REQUIRE(responses.size() == 0);

    nlohmann::json jj;
    jj["message_type"]       = MessageType::POSITION;
    jj["code"]               = "test room";
    jj["content"]["peer_id"] = client->id();
    responses                = client->send(jj);

    REQUIRE(responses.size() == 1);
    auto &response = responses.front();
    REQUIRE(response["message_type"].template get<MessageType>() == MessageType::POSITION);
    REQUIRE(response["content"][client->id()].template get<float>() == Approx(0));
  }

  SECTION("multiple client positions")
  {
    add_client(server);
    add_client(server);
    add_client(server);
    REQUIRE(clients.size() == 4);

    for (auto &&[id, client] : clients)
    {
      client->clear_message_queue();
    }

    std::array<std::tuple<float, float, float>, 4> positions;
    positions[0] = {0, 0, 0};
    positions[1] = {1, 1, 1};
    positions[2] = {-1, -1, -1};
    positions[3] = {10, -4, 17};

    std::array<std::string, 4> client_ids;
    std::transform(clients.begin(), clients.end(), client_ids.begin(), [](auto pair) { return pair.first; });
    std::array<std::array<float, 4>, 4> distances = {{{{0, std::sqrtf(3), std::sqrtf(3), std::sqrtf(405)}},
                                                      {{std::sqrtf(3), 0, std::sqrtf(12), std::sqrtf(362)}},
                                                      {{std::sqrtf(3), std::sqrtf(12), 0, std::sqrtf(454)}},
                                                      {{std::sqrtf(405), std::sqrtf(362), std::sqrtf(454), 0}}}};
    std::array<std::array<float, 4>, 4> volumes   = {
        {{{1, 1 / (std::sqrtf(3) * std::sqrtf(3)), 1 / (std::sqrtf(3) * std::sqrtf(3)), 1 / (std::sqrtf(405) * std::sqrtf(405))}},
         {{1 / (std::sqrtf(3) * std::sqrtf(3)), 1, 1 / (std::sqrtf(12) * std::sqrtf(12)), 1 / (std::sqrtf(362) * std::sqrtf(362))}},
         {{1 / (std::sqrtf(3) * std::sqrtf(3)), 1 / (std::sqrtf(12) * std::sqrtf(12)), 1, 1 / (std::sqrtf(454) * std::sqrtf(454))}},
         {{1 / (std::sqrtf(405) * std::sqrtf(405)),
           1 / (std::sqrtf(362) * std::sqrtf(362)),
           1 / (std::sqrtf(454) * std::sqrtf(454)),
           1}}}};

    std::vector<nlohmann::json> responses;
    for (auto ii = 0; ii < 4; ++ii)
    {
      auto &&[x, y, z] = positions[ii];
      auto &&client    = clients.at(client_ids[ii]);

      nlohmann::json j;
      j["message_type"]        = MessageType::POSITION;
      j["code"]                = "test room";
      j["content"]["position"] = {{"x", x}, {"y", y}, {"z", z}};
      client->clear_message_queue();
      responses = client->send(j);
    }

    // only test distances after last query, because only then are all of the test positions populated correctly
    REQUIRE(responses.size() == 3);
    for (auto &response : responses)
    {
      auto id_ii = client_ids.back();
      auto id_jj = response["peer_id"].template get<std::string>();

      auto idx_ii = std::distance(client_ids.begin(), std::find(client_ids.begin(), client_ids.end(), id_ii));
      auto idx_jj = std::distance(client_ids.begin(), std::find(client_ids.begin(), client_ids.end(), id_jj));

      REQUIRE(response["message_type"].template get<MessageType>() == MessageType::VOLUME);
      REQUIRE(response["content"]["volume"].template get<float>() == Approx(volumes[idx_ii][idx_jj]));
    }

    // check all positions
    responses =
        clients.at(client_ids.back())->send(nlohmann::json({{"message_type", MessageType::POSITION}, {"code", "test room"}}));
    REQUIRE(responses.size() == 1);
    auto response = responses.front();
    REQUIRE(response["message_type"].template get<MessageType>() == MessageType::POSITION);
    for (auto id_peer : client_ids)
    {
      auto idx_client = std::distance(client_ids.begin(), std::find(client_ids.begin(), client_ids.end(), client_ids.back()));
      auto idx_peer   = std::distance(client_ids.begin(), std::find(client_ids.begin(), client_ids.end(), id_peer));

      REQUIRE(response["content"][id_peer].template get<float>() == Approx(distances[idx_client][idx_peer]));
    }
  }
}
