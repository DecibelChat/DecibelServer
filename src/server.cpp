#include "server.hpp"

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <utility>

template <>
struct fmt::formatter</* websocket_server::connection_type*/ websocketpp::connection_hdl> : formatter<void *>
{
  template <typename FormatContext>
  auto format(/* websocket_server::connection_type*/ websocketpp::connection_hdl handle, FormatContext &ctx)
  {
    return fmt::formatter<void *>::format(handle.lock().get(), ctx);
  }
};

namespace websocket_server
{
  using json                = nlohmann::json;
  constexpr auto empty_room = "";

  WSS::WSS(const Parameters &params) : run_debug_logger_(params.verbose)
  {
    using websocketpp::lib::bind;

    server_.init_asio();

    server_.set_message_handler([this](auto handle, auto message) { message_handler(handle, message); });
    server_.set_http_handler([this](auto handle) { http_handler(handle); });
    server_.set_close_handler([this](auto handle) { remove_client_from_room(handle); });
    server_.set_tls_init_handler(
        [this, params](auto handle) { return tls_init_handler(handle, params.key_file, params.cert_file); });

    server_.listen(params.port);
    server_.start_accept();

    debug_logger_ = std::jthread{[this]() {
      constexpr auto interval = std::chrono::seconds{1};
      while (run_debug_logger_.load(std::memory_order_acquire))
      {
        fmt::print("[{:%F %T}] Rooms:\n", fmt::localtime(std::time(nullptr)));

        for (const auto &room : rooms_)
        {
          fmt::print("  [{}] : ", room.first);

          fmt::print("{{{}}}\n", fmt::join(room.second, ", "));
        }
        std::this_thread::sleep_for(interval);
      }
    }};
  }

  WSS::~WSS()
  {
    run_debug_logger_.store(false, std::memory_order_release);
  }

  void WSS::start()
  {
    {
      server_.run();
    }
  }

  void WSS::message_handler(connection_type handle, message_pointer_type message)
  {
    auto connection = server_.get_con_from_hdl(handle);

    auto parsed_data = json::parse(message->get_payload());

    fmt::print("{}\n", parsed_data.dump(2));

    auto room_id = parsed_data.at("code").get<room_id_type>();

    auto [current_client, added_to_room] = add_client_to_room(room_id, handle);

    const auto &current_room = rooms_.at(room_id);

    for (auto peer = current_room.begin(); peer != current_room.end(); ++peer)
    {
      if (current_client != peer)
      {
        server_.send(*peer, message);
      }
    }
  }

  std::pair<WSS::room_type::iterator, bool> WSS::add_client_to_room(const room_id_type &room_id, connection_type handle)
  {
    auto &current_room = rooms_[room_id];

    client_mapping_[handle] = room_id;
    return current_room.insert(handle);
  }

  void WSS::remove_client_from_room(connection_type handle)
  {
    auto room_id = client_mapping_.at(handle);

    fmt::print("removed client {} from room {}", handle.lock().get(), room_id);
    rooms_[room_id].erase(handle);
    client_mapping_.erase(handle);

    auto room_closed = close_if_empty(room_id);
  }

  bool WSS::close_if_empty(const room_id_type &room_id)
  {
    if (rooms_.contains(room_id) && rooms_[room_id].empty())
    {
      rooms_.erase(room_id);
      return true;
    }

    return false;
  }

  void WSS::http_handler(connection_type handle)
  {
    auto connection = server_.get_con_from_hdl(handle);

    connection->set_status(websocketpp::http::status_code::accepted);
  }

  WSS::tls_context_pointer_type WSS::tls_init_handler(connection_type handle, const fs::path &keyfile, const fs::path &certfile)
  {
    namespace asio                           = websocketpp::lib::asio;
    tls_context_pointer_type context_pointer = websocketpp::lib::make_shared<tls_context_type>(asio::ssl::context::sslv23);

    client_mapping_.insert({handle, empty_room});
    try
    {
      context_pointer->set_options(asio::ssl::context::default_workarounds | asio::ssl::context::no_sslv2 |
                                   asio::ssl::context::no_sslv3 | asio::ssl::context::no_tlsv1 |
                                   asio::ssl::context::single_dh_use);

      context_pointer->use_certificate_file(fs::canonical(certfile).string(), asio::ssl::context::pem);
      context_pointer->use_private_key_file(fs::canonical(keyfile).string(), asio::ssl::context::pem);
    }
    catch (const std::exception &e)
    {
      fmt::print("Exception thrown while initializing TLS:\n{}", e.what());
    }

    return context_pointer;
  }
} // namespace websocket_server
