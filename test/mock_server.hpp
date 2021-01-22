#include <server.hpp>

#include <nlohmann/json.hpp>

#include <queue>
#include <vector>

namespace websocket_server
{
  namespace mock
  {
    class MockServer;
    class MockClient;

    class MockServer : public WSS
    {
    public:
      MockServer(const Parameters &p);

      void start();

      void open(MockClient &client);

      void send(MockClient &client, std::string &input);

      friend MockClient;
    };

    class MockClient : public MockServer::socket_type
    {
    public:
      MockClient(MockServer &server);

      void open();
      std::vector<nlohmann::json> send(const nlohmann::json &j);

      std::vector<nlohmann::json> get_message_queue();
      void clear_message_queue();

      void *getUserData() override;
      bool send(MockServer::message_view_type message, uWS::OpCode opCode = uWS::OpCode::BINARY, bool compress = false) override;

      ClientInfo::client_id_type id();

    private:
      MockServer::user_data_type user_data_;
      MockServer &server_;

      std::queue<MockServer::message_type> messages_;
    };

  } // namespace mock
} // namespace websocket_server