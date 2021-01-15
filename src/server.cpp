#include "server.hpp"

#include "MessageType.hpp"

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <utility>

template <>
struct fmt::formatter<websocket_server::WSS::connection_type> : formatter<std::string_view>
{
  template <typename FormatContext>
  auto format(websocket_server::WSS::connection_type handle, FormatContext &ctx)
  {
    return fmt::formatter<std::string_view>::format(handle->getRemoteAddressAsText(), ctx);
  }
};

namespace std
{
  template <>
  struct hash<websocket_server::WSS::connection_type>
  {
    size_t operator()(const websocket_server::WSS::connection_type &handle) const
    {
      // return std::hash<std::string>()(fmt::format("{}", handle));
      return std::hash<std::string_view>()(handle->getRemoteAddress());
    }
  };
} // namespace std

namespace websocket_server
{
  using json                      = nlohmann::json;
  constexpr auto message_type_key = "message_type";
  constexpr auto peer_id_key      = "peer_id";
  constexpr auto data_key         = "content";

  WSS::WSS(const Parameters &params) :
      server_([params]() {
        if constexpr (using_TLS)
        {
          return uWS::SocketContextOptions{.key_file_name  = params.key_file.string().c_str(),
                                           .cert_file_name = params.cert_file.string().c_str()};
        }
        return uWS::SocketContextOptions{};
      }()),
      run_debug_logger_(params.verbose)
  {
    server_.ws<user_data_type>("/*",
                               {.open    = [this](auto ws) { http_handler(ws); },
                                .message = [this](auto ws, auto message, auto op_code) { message_handler(ws, message); },
                                .close   = [this](auto ws, auto code, auto message) { remove_client_from_room(ws); }});
    server_.listen(params.port, [verbose = params.verbose, port = params.port](auto listen_socket) {
      if (verbose)
      {
        if (listen_socket)
        {
          fmt::print(fg(fmt::color::lime_green), "initialized wss server on port: {}\n", port);
        }
        else
        {
          fmt::print(fg(fmt::color::orange_red), "ERROR: unable to initialize wss server on port: {}\n", port);
        }
      }
    });

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

    server_.run();
  }

  void WSS::message_handler(connection_type handle, message_view_type message)
  {
    // auto connection = std::visit([handle](auto &&server) { return server.get_con_from_hdl(handle); }, server_);

    auto parsed_data = json::parse(message.data());

    if (run_debug_logger_)
    {
      fmt::print(fg(fmt::color::yellow), "{}\n", parsed_data.dump(2));
    }

    auto room_id = parsed_data.at("code").get<room_id_type>();

    auto [current_client, added_to_room] = add_client_to_room(room_id, handle);

    const auto &current_room = rooms_.at(room_id);

    parsed_data[peer_id_key] = client_mapping_[*current_client].id();
    auto new_message         = parsed_data.dump();

    for (auto peer = current_room.begin(); peer != current_room.end(); ++peer)
    {
      if (current_client != peer)
      {
        (*peer)->send(new_message, uWS::OpCode::TEXT, compress_outgoing_messages);
      }
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
      handle->send(client_reply_message.dump(), uWS::OpCode::TEXT, compress_outgoing_messages);
      // std::visit([handle, client_reply_message](
      //               auto &&server) { server.send(handle, client_reply_message.dump(), websocketpp::frame::opcode::TEXT); },
      //           server_);

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

    const auto &client      = client_mapping_.at(handle);
    const auto &client_uuid = client.id();
    const auto &room_id     = client.room();

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
      (*peer)->send(message.dump(), uWS::OpCode::TEXT, compress_outgoing_messages);
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

  void WSS::http_handler(connection_type handle)
  {
    user_data_type user_data = *static_cast<user_data_type *>(handle->getUserData());

    if (run_debug_logger_)
    {
      fmt::print(fg(fmt::color::hot_pink),
                 "received new connection request:\n[{}:{}]\n{}\n",
                 handle->getRemoteAddress(),
                 handle->getRemoteAddressAsText(),
                 user_data);
    }
    // std::visit(
    //    [handle](auto &&server) {
    //      auto connection = server.get_con_from_hdl(handle);
    //      connection->set_status(websocketpp::http::status_code::accepted);
    //    },
    //    server_);
  }
} // namespace websocket_server
