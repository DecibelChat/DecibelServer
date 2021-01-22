#include "mock_server.hpp"

namespace websocket_server::mock
{
  MockClient::MockClient(MockServer &server) : server_(server)
  {
    open();
  }

  void MockClient::open()
  {
    server_.open(*this);
  }

  std::vector<nlohmann::json> MockClient::send(const nlohmann::json &j)
  {
    auto outbound = j.dump();
    server_.send(*this, outbound);

    return get_message_queue();
  }

  void *MockClient::getUserData()
  {
    return static_cast<void *>(&user_data_);
  }

  bool MockClient::send(MockServer::message_view_type message, uWS::OpCode opCode, bool compress)
  {
    messages_.push(std::string{message});
    return true;
  }

  std::vector<nlohmann::json> MockClient::get_message_queue()
  {
    std::vector<nlohmann::json> results;
    results.reserve(messages_.size());
    while (!messages_.empty())
    {
      results.push_back(nlohmann::json::parse(messages_.front()));
      messages_.pop();
    }
    return results;
  }

  void MockClient::clear_message_queue()
  {
    decltype(messages_) empty;
    std::swap(empty, messages_);
  }

  ClientInfo::client_id_type MockClient::id()
  {
    return user_data_;
  }

  MockServer::MockServer(const Parameters &p) : WSS(p)
  {
  }

  void MockServer::start()
  {
  }

  void MockServer::open(MockClient &client)
  {
    http_handler(&client);
  }

  void MockServer::send(MockClient &client, std::string &input)
  {
    message_handler(&client, input);
  }

} // namespace websocket_server::mock