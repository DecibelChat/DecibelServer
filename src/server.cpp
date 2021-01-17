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
  using json                      = nlohmann::json;
  constexpr auto message_type_key = "message_type";
  constexpr auto peer_id_key      = "peer_id";
  constexpr auto data_key         = "content";

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
      run_debug_logger_(params.verbose)
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
                                         else if (run_debug_logger_)
                                         {
                                           fmt::print(fg(fmt::color::orange_red),
                                                      "ERROR: Cannot handle received message of type: {}\n{}\n",
                                                      static_cast<int>(op_code),
                                                      message);
                                         }
                                       },
                                   .close =
                                       [this](auto ws, auto code, auto message) {
                                         if (run_debug_logger_)
                                         {
                                           fmt::print(fg(fmt::color::dark_turquoise), "{}: {}\n", code, message);
                                         }
                                         remove_client_from_room(ws);
                                       },
                               });
    server_.listen(params.port, [display = &run_debug_logger_, port = params.port](auto listen_socket) {
      if (display)
      {
        if (listen_socket)
        {
          log(spdlog::level::info, "initialized wss server on port: {}\n", port);
          fmt::print(fg(fmt::color::lime_green), "initialized wss server on port: {}\n", port);
        }
        else
        {
          log(spdlog::level::critical, "ERROR: unable to initialize wss server on port: {}\n", port);
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
      if (filename.empty())
      {
        filename = fs::temp_directory_path() / "decibel_server" / "log.txt";
      }

      if (max_mb > 0)
      {
        auto max_size            = static_cast<std::size_t>(megabyte * max_mb);
        constexpr auto max_files = 2;
        return spdlog::rotating_logger_mt(logger_name, filename.string(), max_size, max_files);
      }

      return spdlog::basic_logger_mt(logger_name, filename.string(), true);
    }(parameters.log_file, parameters.max_log_mb);

    console->set_level(parameters.verbose ? spdlog::level::debug : spdlog::level::info);
    errors->set_level(spdlog::level::err);
    file_logger->set_level(spdlog::level::trace);

    spdlog::set_pattern("[%Y-%m-%d %T.%F] [%l] %v"); // [YYYY-MM-DD HH:MM:SS.nano] [level] message
  }

  template <typename LogLevel, class... Args>
  void WSS::log(LogLevel level, Args &&...args)
  {
    using first_type = typename std::tuple_element<0, std::tuple<Args...>>::type;
    if constexpr (std::is_same_v<first_type, fmt::color>)
    {
      [](auto &&level, auto &&color, auto &&...remaining_args) {
        spdlog::get(logger_name_console)
            ->log(level, fmt::format(fg(color), std::forward<decltype(remaining_args)>(remaining_args)...));
        spdlog::get(logger_name_error)
            ->log(level, fmt::format(fg(color), std::forward<decltype(remaining_args)>(remaining_args)...));
        spdlog::get(logger_name_file)->log(level, std::forward<decltype(remaining_args)>(remaining_args)...);
      }(level, std::forward<Args>(args)...);
    }
    else
    {
      spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) { l->log(level, std::forward<Args>(args)...); });
    }
  }

  void WSS::message_handler(connection_type handle, message_view_type message)
  {
    auto parsed_data = json::parse(std::string{message});

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

      user_data(handle) = client_mapping_[handle].id();

      if (run_debug_logger_)
      {
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
    if (run_debug_logger_)
    {
      fmt::print(fg(fmt::color::hot_pink),
                 "received new connection request:\n[{}:{}]\n",
                 handle->getRemoteAddress(),
                 handle->getRemoteAddressAsText());
    }
  }

} // namespace websocket_server
