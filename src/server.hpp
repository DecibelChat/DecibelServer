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
  constexpr bool using_TLS = true;
#else
  constexpr bool using_TLS = false;
#endif
  constexpr bool compress_outgoing_messages = true;

  struct Parameters
  {
    std::uint16_t port;

    fs::path cert_file;
    fs::path key_file;

    bool verbose;
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

  public:
    using connection_type = socket_type *;

  private:
    using user_data_type = std::string;
    // using connection_comparator = std::owner_less<connection_type>;
    struct connection_comparator
    {
      bool operator()(const connection_type &lhs, const connection_type &rhs) const
      {
        return lhs->getRemoteAddress() < rhs->getRemoteAddress();
      }
    };
    // using tls_context_type         = websocketpp::lib::asio::ssl::context;
    // using tls_context_pointer_type = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;
    using message_type      = std::string;
    using message_view_type = std::string_view;

    using client_type          = ClientInfo;
    using room_id_type         = client_type::room_id_type;
    using room_type            = std::set<connection_type, connection_comparator>;
    using rooms_container_type = std::unordered_map<room_id_type, room_type>;

    using client_lookup_type = std::map<connection_type, client_type, connection_comparator>;

#ifdef __cpp_lib_jthread
    using thread_type = std::jthread;
#else
    using thread_type = std::thread;
#endif

    void message_handler(connection_type handle, message_view_type message);

    std::pair<room_type::iterator, bool> add_client_to_room(const room_id_type &room_id, connection_type handle);
    void remove_client_from_room(connection_type handle);
    bool close_if_empty(const room_id_type &room_id);

    void http_handler(connection_type handle);

    server_backend_type server_;
    rooms_container_type rooms_;
    client_lookup_type client_mapping_;

    thread_type debug_logger_;
    std::atomic<bool> run_debug_logger_;
  };
} // namespace websocket_server
