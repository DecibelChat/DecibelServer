#pragma once

#include <App.h>

#include "ClientInfo.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <thread>
#include <unordered_map>
#include <variant>

namespace websocket_server
{
  namespace fs = std::filesystem;

#ifdef WS_NO_TLS
  constexpr bool using_TLS = false;
#else
  constexpr bool using_TLS = true;
#endif
  constexpr bool compress_outgoing_messages = true;

  struct Parameters
  {
    std::uint16_t port;

    fs::path cert_file;
    fs::path key_file;

    bool verbose;

    float max_log_mb;
    fs::path log_file;
  };

  class WSS
  {
  public:
    explicit WSS(const Parameters &params);
    ~WSS();

    // for now, disable copy and move semantics, because underlying websocketpp::server<> types provide neither ability.
    WSS(const WSS &)     = delete;
    WSS(WSS &&) noexcept = delete;
    WSS &operator=(const WSS &) = delete;
    WSS &operator=(WSS &&) noexcept = delete;

    void start();

  private:
    using server_backend_type = uWS::TemplatedApp<using_TLS>;
    using socket_type         = uWS::WebSocket<using_TLS, true>;
    using message_type        = std::string;
    using message_view_type   = std::string_view;

    using client_type    = ClientInfo;
    using client_id_type = client_type::client_id_type;
    using room_id_type   = client_type::room_id_type;

  public:
    using connection_type = socket_type *;
    using user_data_type  = client_id_type;

    static user_data_type &user_data(connection_type handle);

  private:
    struct connection_comparator
    {
      bool operator()(const connection_type &lhs, const connection_type &rhs) const
      {
        return user_data(lhs) < user_data(rhs);
      }
    };
    using client_lookup_type = std::map<connection_type, client_type, connection_comparator>;

  public:
    using room_type            = std::set<connection_type, connection_comparator>;
    using rooms_container_type = std::unordered_map<room_id_type, room_type>;

  private:
#ifdef __cpp_lib_jthread
    using thread_type = std::jthread;
#else
    using thread_type = std::thread;
#endif
    static constexpr auto logger_name_console = "decibel console";
    static constexpr auto logger_name_error   = "decibel errors";
    static constexpr auto logger_name_file    = "decibel log file";

    static void initialize_loggers(const Parameters &);

    template <typename LogLevel, class... Args>
    static void log(LogLevel, Args &&...);

    void message_handler(connection_type handle, message_view_type message);

    std::pair<room_type::iterator, bool> add_client_to_room(const room_id_type &room_id, connection_type handle);
    void remove_client_from_room(connection_type handle);
    bool close_if_empty(const room_id_type &room_id);

    void http_handler(connection_type handle);

    const std::string key_;
    const std::string cert_;

    server_backend_type server_;
    rooms_container_type rooms_;
    client_lookup_type client_mapping_;

    thread_type debug_logger_;
    std::atomic<bool> run_debug_logger_;
  };
} // namespace websocket_server
