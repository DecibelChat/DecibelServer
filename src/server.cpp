#include "server.hpp"

#include "MessageType.hpp"

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <utility>

template <>
struct fmt::formatter<websocket_server::WSS::connection_type> : formatter<void *>
{
  template <typename FormatContext>
  auto format(websocket_server::WSS::connection_type handle, FormatContext &ctx)
  {
    return fmt::formatter<void *>::format(handle.lock().get(), ctx);
  }
};

namespace std
{
  template <>
  struct hash<websocket_server::WSS::connection_type>
  {
    size_t operator()(const websocketpp::connection_hdl &handle) const
    {
      return std::hash<std::string>()(fmt::format("{}", handle));
    }
  };
} // namespace std

namespace websocket_server
{
  using json                      = nlohmann::json;
  constexpr auto message_type_key = "message_type";
  constexpr auto peer_id_key      = "peer_id";
  constexpr auto data_key         = "content";

  WSS::WSS(const Parameters &params) : run_debug_logger_(params.verbose), insecure_(params.insecure)
  {
    using websocketpp::lib::bind;

    if (insecure_)
    {
      server_.emplace<insecure_server_type>();
    }
    else
    {
      server_.emplace<secure_server_type>();
    }

    std::visit([](auto &&server) { server.init_asio(); }, server_);
    std::visit([](auto &&server) { server.set_reuse_addr(true); }, server_);

    std::visit(
        [this](auto &&server) {
          server.set_message_handler([this](auto handle, auto message) { message_handler(handle, message); });
        },
        server_);
    std::visit([this](auto &&server) { server.set_http_handler([this](auto handle) { http_handler(handle); }); }, server_);
    std::visit([this](auto &&server) { server.set_close_handler([this](auto handle) { remove_client_from_room(handle); }); },
               server_);
    std::visit(
        [this, params](auto &&server) {
          if constexpr (std::is_same_v<std::decay_t<decltype(server)>, secure_server_type>)
          {
            server.set_tls_init_handler(
                [this, params](auto handle) { return tls_init_handler(handle, params.key_file, params.cert_file); });
          }
        },
        server_);

    std::visit([port = params.port](auto &&server) { server.listen(port); }, server_);
    std::visit([](auto &&server) { server.start_accept(); }, server_);

    if (params.verbose)
    {
      fmt::print(fg(fmt::color::lime_green), "initialized wss server on port: {}\n", params.port);
    }

    debug_logger_ = thread_type{[this]() {
      constexpr auto interval = std::chrono::seconds{1};
      while (run_debug_logger_.load(std::memory_order_acquire))
      {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        fmt::print(fg(fmt::color::violet), "[{:%F %T}] Rooms:\n", fmt::localtime(now));

        for (const auto &room : rooms_)
        {
          fmt::print(fg(fmt::color::violet), "  [{}] : ", room.first);

          fmt::print(fg(fmt::color::violet), "{{{}}}\n", fmt::join(room.second, ", "));
        }
        std::this_thread::sleep_for(interval);
      }
    }};
  }

  WSS::~WSS()
  {
    run_debug_logger_.store(false, std::memory_order_release);

#ifndef __cpp_lib_jthread
    debug_logger_.join();
#endif
  }

  void WSS::start()
  {
    if (run_debug_logger_)
    {
      fmt::print(fg(fmt::color::cyan), "starting server.\n");
    }

    std::visit([](auto &&server) { server.run(); }, server_);
  }

  void WSS::message_handler(connection_type handle, message_pointer_type message)
  {
    // auto connection = std::visit([handle](auto &&server) { return server.get_con_from_hdl(handle); }, server_);

    auto parsed_data = json::parse(message->get_payload());

    if (run_debug_logger_)
    {
      fmt::print(fg(fmt::color::yellow), "{}\n", parsed_data.dump(2));
    }

    auto message_type = parsed_data.at("message_type").get<MessageType>();

    auto room_id = parsed_data.at("code").get<room_id_type>();

    if (message_type == MessageType::SERVER || message_type == MessageType::CANDIDATE)
    {
      auto [current_client, added_to_room] = add_client_to_room(room_id, handle);

      const auto &current_room = rooms_.at(room_id);

      parsed_data[peer_id_key] = client_mapping_[*current_client].id();
      message->set_payload(parsed_data.dump());

      for (auto peer = current_room.begin(); peer != current_room.end(); ++peer)
      {
        if (current_client != peer)
        {
          std::visit([peer, message](auto &&server) { server.send(*peer, message); }, server_);
        }
      }
    }
    else if (message_type == MessageType::POSITION)
    {
      const auto &current_room = rooms_.at(room_id);
      auto &current_client     = client_mapping_[handle];

      auto position = parsed_data.at(data_key).at("position");

      auto x = position.contains("x") ? position.at("x").get<ClientInfo::position_value_type>() : 0.f;
      auto y = position.contains("y") ? position.at("y").get<ClientInfo::position_value_type>() : 0.f;
      auto z = position.contains("z") ? position.at("z").get<ClientInfo::position_value_type>() : 0.f;

      json volume_update = {{peer_id_key, current_client.id()},
                            {message_type_key, message_type_to_string.at(MessageType::VOLUME)},
                            {data_key, {{"volume", 0.f}}}};

      if (run_debug_logger_)
      {
        fmt::print(fg(fmt::color::yellow), "{}\n", volume_update.dump(2));
      }

      if (current_client.update_position({x, y, z}))
      {
        for (auto peer = current_room.begin(); peer != current_room.end(); ++peer)
        {
          if (!connection_comparator()(handle, *peer))
          {
            const auto &peer_client = client_mapping_.at(*peer);
            auto distance           = current_client.distance(peer_client);
            auto volume             = calculate_volume(distance);

            volume_update[data_key]["volume"] = volume;

            volume_update[peer_id_key] = current_client.id();
            message->set_payload(volume_update.dump());
            std::visit([peer, message](auto &&server) { server.send(*peer, message); }, server_);
            if (run_debug_logger_)
            {
              fmt::print(fg(fmt::color::coral), "{}\n", volume_update.dump(2));
            }

            volume_update[peer_id_key] = peer_client.id();
            message->set_payload(volume_update.dump());
            std::visit([handle, message](auto &&server) { server.send(handle, message); }, server_);
            if (run_debug_logger_)
            {
              fmt::print(fg(fmt::color::coral), "{}\n", volume_update.dump(2));
            }
          }
        }
      }
    }
    else
    {
      fmt::print(fg(fmt::color::orange_red),
                 "Unable to handle received message of type MessageType::{}",
                 message_type_to_string.at(message_type));
    }
  }

  std::pair<WSS::room_type::iterator, bool> WSS::add_client_to_room(const room_id_type &room_id, connection_type handle)
  {
    auto &current_room = rooms_[room_id];

    // add client to room if not already present
    auto result = current_room.insert(handle);

    if (result.second)
    {
      // update internal bookkeeping of client's room
      client_mapping_[handle].assign_room(room_id);

      // notify client of their own UUID
      json client_reply_message = {{peer_id_key, client_mapping_[handle].id()},
                                   {message_type_key, message_type_to_string.at(MessageType::SERVER)},
                                   {data_key, "your id"}};
      std::visit([handle, client_reply_message](
                     auto &&server) { server.send(handle, client_reply_message.dump(), websocketpp::frame::opcode::TEXT); },
                 server_);

      if (run_debug_logger_)
      {
        // auto connection = std::visit([handle](auto &&server) { return server.get_con_from_hdl(handle); }, server_);
        auto uuid = client_mapping_[handle].id();

        fmt::print(fg(fmt::color::dark_turquoise), "Added connection: [room: {}, uuid: {}]\n", room_id, uuid);

        fmt::print(fg(fmt::color::aquamarine), "{}\n", client_reply_message.dump(2));
      }
    }

    return result;
  }

  void WSS::remove_client_from_room(connection_type handle)
  {
    constexpr auto uuid_key       = peer_id_key;
    constexpr auto delete_message = "delete";

    const auto &client     = client_mapping_.at(handle);
    const auto client_uuid = client.id();
    const auto room_id     = client.room();

    json message = {
        {uuid_key, client_uuid}, {message_type_key, message_type_to_string.at(MessageType::SERVER)}, {data_key, delete_message}};

    auto &current_room = rooms_.at(room_id);
    current_room.erase(handle);
    client_mapping_.erase(handle);

    if (run_debug_logger_)
    {
      fmt::print(fg(fmt::color::dark_turquoise), "removed client {} from room {}\n", client_uuid, room_id);

      fmt::print(fg(fmt::color::aquamarine), "{}\n", message.dump(2));
    }

    for (auto peer = current_room.begin(); peer != current_room.end(); ++peer)
    {
      std::visit([peer, message](auto &&server) { server.send(*peer, message.dump(), websocketpp::frame::opcode::TEXT); },
                 server_);
    }

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

  WSS::client_type::position_value_type WSS::calculate_volume(client_type::position_value_type distance) const
  {
    /*
     * This will likely need tweaking for UX, but as a staring point:
     * - Sound intensity is inversely proportional to the square of distance.
     * - Output is desired in the format (as governed by HTML video API) to be a fractional value in the range [0,1]
     */
    return distance <= 1.f ? 1.f : 1 / (distance * distance);
  }

  void WSS::http_handler(connection_type handle)
  {
    std::visit(
        [handle](auto &&server) {
          auto connection = server.get_con_from_hdl(handle);
          connection->set_status(websocketpp::http::status_code::accepted);
        },
        server_);
  }

  WSS::tls_context_pointer_type WSS::tls_init_handler(connection_type handle, const fs::path &keyfile, const fs::path &certfile)
  {
    namespace asio                           = websocketpp::lib::asio;
    tls_context_pointer_type context_pointer = websocketpp::lib::make_shared<tls_context_type>(asio::ssl::context::sslv23);

    client_mapping_.try_emplace(handle);
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
      fmt::print(fg(fmt::color::orange_red), "Exception thrown while initializing TLS:\n{}", e.what());
    }

    return context_pointer;
  }
} // namespace websocket_server
