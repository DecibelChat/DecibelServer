#include "server.hpp"

#include "MessageType.hpp"

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <utility>

template <>
struct fmt::formatter<websocket_server::WSS::connection_type> : formatter<std::string>
{
  template <typename FormatContext>
  auto format(websocket_server::WSS::connection_type handle, FormatContext &ctx)
  {
    return fmt::formatter<std::string>::format(websocket_server::WSS::user_data(handle), ctx);
  }
};

template <>
struct fmt::formatter<websocket_server::WSS::rooms_container_type>
{
  std::string indent;

  constexpr auto parse(format_parse_context &ctx)
  {
    auto it  = ctx.begin();
    auto end = ctx.end();
    if (it != end)
    {
      try
      {
        auto indent_size = std::stoi(it);
        for (auto ii = 0; ii < indent_size; ++ii)
        {
          indent += " ";
        }
        ++it;
      }
      catch (...)
      {
        throw format_error("invalid format");
      }
    }

    // Check if reached the end of the range:
    if (it != end && *it != '}')
    {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }
  template <typename FormatContext>
  auto format(const websocket_server::WSS::rooms_container_type &rooms, FormatContext &ctx)
  {
    for (auto room_iter = rooms.begin(); room_iter != rooms.end(); ++room_iter)
    {
      if (room_iter != rooms.begin())
      {
        format_to(ctx.out(), "\n");
      }

      format_to(ctx.out(), "{}[{}] : {{{}}}", indent, room_iter->first, fmt::join(room_iter->second, ", "));
    }

    return ctx.out();
  }
};

template <>
struct fmt::formatter<nlohmann::json> : formatter<std::string>
{
  inline static std::atomic<int> indent = -1;

  template <typename FormatContext>
  auto format(const nlohmann::json &j, FormatContext &ctx)
  {
    return fmt::formatter<std::string>::format(j.dump(indent.load()), ctx);
  }
};

namespace std
{
  template <>
  struct hash<websocket_server::WSS::connection_type>
  {
    size_t operator()(const websocket_server::WSS::connection_type &handle) const
    {
      return std::hash<std::string>()(websocket_server::WSS::user_data(handle));
    }
  };
} // namespace std

namespace websocket_server
{
  using json                         = nlohmann::json;
  constexpr auto message_type_key    = "message_type";
  constexpr auto peer_id_key         = "peer_id";
  constexpr auto data_key            = "content";
  constexpr auto logger_name_console = "decibel console";
  constexpr auto logger_name_error   = "decibel errors";
  constexpr auto logger_name_file    = "decibel log file";

  WSS::WSS(const Parameters &params) :
      key_(params.key_file.string()),
      cert_(params.cert_file.string()),
      server_([this]() {
        if constexpr (using_TLS)
        {
          return uWS::SocketContextOptions{.key_file_name = key_.c_str(), .cert_file_name = cert_.c_str()};
        }
        return uWS::SocketContextOptions{};
      }()),
      run_debug_logger_(true)
  {
    initialize_loggers(params);

    server_.ws<user_data_type>("/*",
                               {
                                   .compression = (compress_outgoing_messages) ? uWS::CompressOptions::SHARED_COMPRESSOR :
                                                                                 uWS::CompressOptions::DISABLED,
                                   .open        = [this](auto ws) { http_handler(ws); },
                                   .message =
                                       [this](auto ws, auto message, auto op_code) {
                                         if (op_code == uWS::OpCode::TEXT)
                                         {
                                           message_handler(ws, message);
                                         }
                                         else
                                         {
                                           log(spdlog::level::warn,
                                               fmt::color::orange_red,
                                               "cannot handle received message of type: {} [{}]",
                                               static_cast<int>(op_code),
                                               message);
                                         }
                                       },
                                   .close =
                                       [this](auto ws, auto code, auto message) {
                                         log(spdlog::level::trace, fmt::color::dark_turquoise, "{}: {}", code, message);

                                         remove_client_from_room(ws);
                                       },
                               });
    server_.listen(params.port, [port = params.port](auto listen_socket) {
      if (listen_socket)
      {
        log(spdlog::level::info, fmt::color::lime_green, "initialized wss server on port: {}", port);
      }
      else
      {
        log(spdlog::level::critical, fmt::color::orange_red, "unable to initialize wss server on port: {}", port);
      }
    });

    debug_logger_ = thread_type{[this]() {
      constexpr auto interval = std::chrono::seconds{1};
      while (run_debug_logger_.load(std::memory_order_acquire))
      {
        log(spdlog::level::debug, fmt::color::violet, "Rooms:{}{:2}", rooms_.empty() ? "" : "\n", rooms_);
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
    log(spdlog::level::info, fmt::color::cyan, "starting server.");

    server_.run();
  }

  WSS::user_data_type &WSS::user_data(connection_type handle)
  {
    user_data_type &user_data = *static_cast<user_data_type *>(handle->getUserData());

    return user_data;
  }

  void WSS::initialize_loggers(const Parameters &parameters)
  {
    constexpr auto logger_flush_period = std::chrono::seconds{1};
    constexpr auto megabyte            = 1024 * 1024;

    spdlog::drop_all(); // remove default logger
    spdlog::flush_every(logger_flush_period);

    auto console = spdlog::stdout_color_mt(logger_name_console);

    auto errors = spdlog::stderr_color_mt(logger_name_error);

    auto file_logger = [logger_name = logger_name_file](auto filename, auto max_mb) {
      if (max_mb > 0)
      {
        auto max_size            = static_cast<std::size_t>(megabyte * static_cast<double>(max_mb));
        constexpr auto max_files = 2;
        return spdlog::rotating_logger_mt(logger_name, filename.string(), max_size, max_files, true);
      }

      return spdlog::basic_logger_mt(logger_name, filename.string(), true);
    }(parameters.log_file, parameters.max_log_mb);

    console->set_level([](int level) {
      if (level <= 0)
      {
        return spdlog::level::err;
      }
      else if (level == 1)
      {
        return spdlog::level::warn;
      }
      else if (level == 2)
      {
        return spdlog::level::info;
      }
      else if (level == 3)
      {
        return spdlog::level::debug;
      }

      return spdlog::level::trace;
    }(parameters.verbosity));
    errors->set_level(spdlog::level::err);
    file_logger->set_level(spdlog::level::trace);

    spdlog::set_pattern("[%Y-%m-%d %T.%F] [%l] %v"); // [YYYY-MM-DD HH:MM:SS.nano] [level] message

    log(spdlog::level::info, "logging to {}", parameters.log_file.string());
  }

  template <typename LogLevel, class... Args>
  void WSS::log(LogLevel level, Args &&...args)
  {
    using first_type  = typename std::tuple_element<0, std::tuple<Args...>>::type;
    auto &indent_size = fmt::formatter<nlohmann::json>::indent;

    if constexpr (std::is_same_v<first_type, fmt::color>)
    {
      [&indent_size](auto &&level, auto &&color, auto &&...remaining_args) {
        indent_size.store(2);
        spdlog::get(logger_name_console)
            ->log(level, fmt::format(fg(color), std::forward<decltype(remaining_args)>(remaining_args)...));
        spdlog::get(logger_name_error)
            ->log(level, fmt::format(fg(color), std::forward<decltype(remaining_args)>(remaining_args)...));
        indent_size.store(-1);
        spdlog::get(logger_name_file)->log(level, std::forward<decltype(remaining_args)>(remaining_args)...);
      }(level, std::forward<Args>(args)...);
    }
    else
    {
      indent_size.store(2);
      spdlog::get(logger_name_console)->log(level, std::forward<Args>(args)...);
      spdlog::get(logger_name_error)->log(level, std::forward<Args>(args)...);
      indent_size.store(-1);
      spdlog::get(logger_name_file)->log(level, std::forward<Args>(args)...);
    }
  }

  void WSS::message_handler(connection_type handle, message_view_type message)
  {
    auto parsed_data = json::parse(std::string{message});

    log(spdlog::level::trace, fmt::color::yellow, "{}", parsed_data);

    auto message_type = parsed_data.at(message_type_key).get<MessageType>();

    auto room_id = parsed_data.at("code").get<room_id_type>();

    if (message_type == MessageType::SERVER || message_type == MessageType::CANDIDATE || message_type == MessageType::SDP)
    {
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
    else if (message_type == MessageType::POSITION)
    {
      const auto &current_room = rooms_.at(room_id);
      auto &current_client     = client_mapping_[handle];

      if (parsed_data.contains(data_key) && parsed_data.at(data_key).contains("position"))
      {
        // update handle's position and inform peers
        auto position = parsed_data.at(data_key).at("position");

        auto x = position.contains("x") ? position.at("x").get<ClientInfo::position_value_type>() : 0.f;
        auto y = position.contains("y") ? position.at("y").get<ClientInfo::position_value_type>() : 0.f;
        auto z = position.contains("z") ? position.at("z").get<ClientInfo::position_value_type>() : 0.f;

        json volume_update = {
            {peer_id_key, current_client.id()}, {message_type_key, MessageType::VOLUME}, {data_key, {{"volume", 0.f}}}};

        log(spdlog::level::trace, fmt::color::yellow, "{}", volume_update);

        if (current_client.update_position({x, y, z}))
        {
          for (auto peer : current_room)
          {
            if (handle != peer)
            {
              const auto &peer_client = client_mapping_.at(peer);
              auto distance           = current_client.distance(peer_client);
              auto volume             = calculate_volume(distance);

              volume_update[data_key]["volume"] = volume;

              volume_update[peer_id_key] = current_client.id();
              peer->send(volume_update.dump(), uWS::OpCode::TEXT, compress_outgoing_messages);
              log(spdlog::level::trace, fmt::color::coral, "{}", volume_update);

              volume_update[peer_id_key] = peer_client.id();
              handle->send(volume_update.dump(), uWS::OpCode::TEXT, compress_outgoing_messages);
              log(spdlog::level::trace, fmt::color::coral, "{}", volume_update);
            }
          }
        }
      }
      else if (parsed_data.contains(data_key) && parsed_data.at(data_key).contains(peer_id_key))
      {
        // send position of requested peer to handle
        auto peer_id = parsed_data.at(data_key).at(peer_id_key).get<client_id_type>();

        // TODO: convert to binary search
        for (auto peer : current_room)
        {
          const auto &peer_client = client_mapping_.at(peer);
          if (peer_client.id() == peer_id)
          {
            json outbound_message = {{message_type_key, MessageType::POSITION},
                                     {data_key, {{peer_id, peer_client.distance(current_client)}}}};
            handle->send(outbound_message.dump(), uWS::OpCode::TEXT, compress_outgoing_messages);
            log(spdlog::level::trace, fmt::color::coral, "[{} -> {}] {}", current_client.id(), peer_id, outbound_message);

            break;
          }
        }
      }
      else
      {
        // send position of all peers to handle
        json outbound_message = {{message_type_key, MessageType::POSITION}, {data_key, json::object()}};
        auto &peer_positions  = outbound_message.at(data_key);

        for (auto peer : current_room)
        {
          const auto &peer_client = client_mapping_.at(peer);

          peer_positions[peer_client.id()] = peer_client.distance(current_client);
        }
        handle->send(outbound_message.dump(), uWS::OpCode::TEXT, compress_outgoing_messages);
        log(spdlog::level::trace, fmt::color::coral, "[ -> {}] {}", current_client.id(), outbound_message);
      }
    }
    else
    {
      log(spdlog::level::warn, fmt::color::orange_red, "unable to handle received message of type MessageType::{}", message_type);
    }
  }

  std::pair<WSS::room_type::iterator, bool> WSS::add_client_to_room(const room_id_type &room_id, connection_type handle)
  {
    if (user_data(handle).empty())
    {
      ClientInfo info;
      user_data(handle) = info.id();

      client_mapping_.insert({handle, info});
    }

    auto &current_room = rooms_[room_id];

    // add client to room if not already present
    auto result = current_room.insert(handle);

    if (result.second)
    {
      // update internal bookkeeping of client's room
      client_mapping_[handle].assign_room(room_id);

      // notify client of their own UUID
      json client_reply_message = {
          {peer_id_key, client_mapping_[handle].id()}, {message_type_key, MessageType::SERVER}, {data_key, "your id"}};
      handle->send(client_reply_message.dump(), uWS::OpCode::TEXT, compress_outgoing_messages);

      auto uuid = client_mapping_[handle].id();

      log(spdlog::level::debug, fmt::color::dark_turquoise, "Added connection: [room: {}, uuid: {}]", room_id, uuid);
      log(spdlog::level::trace, fmt::color::aquamarine, "{}", client_reply_message);
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

    json message = {{uuid_key, client_uuid}, {message_type_key, MessageType::SERVER}, {data_key, delete_message}};

    auto &current_room = rooms_.at(room_id);
    current_room.erase(handle);
    client_mapping_.erase(handle);

    log(spdlog::level::debug, fmt::color::dark_turquoise, "removed client {} from room {}", client_uuid, room_id);
    log(spdlog::level::trace, fmt::color::aquamarine, "{}", message);

    for (auto peer : current_room)
    {
      peer->send(message.dump(), uWS::OpCode::TEXT, compress_outgoing_messages);
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
    log(spdlog::level::debug,
        fmt::color::hot_pink,
        "received new connection request: [{}:{}]",
        handle->getRemoteAddress(),
        handle->getRemoteAddressAsText());
  }

} // namespace websocket_server
