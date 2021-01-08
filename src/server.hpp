#pragma once

#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

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

  struct Parameters
  {
    std::uint16_t port;

    fs::path cert_file;
    fs::path key_file;

    bool verbose;
    bool insecure;
  };

  class WSS
  {
  public:
    WSS(const Parameters &params);
    ~WSS();

    void start();

    using connection_type = websocketpp::connection_hdl;

  private:
    using insecure_server_type     = websocketpp::server<websocketpp::config::asio>;
    using secure_server_type       = websocketpp::server<websocketpp::config::asio_tls>;
    using server_type              = std::variant<insecure_server_type, secure_server_type>;
    using connection_comparator    = std::owner_less<connection_type>;
    using tls_context_type         = websocketpp::lib::asio::ssl::context;
    using tls_context_pointer_type = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;
    using message_type             = websocketpp::config::asio::message_type;
    using message_pointer_type     = message_type::ptr;

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

    void message_handler(connection_type handle, message_pointer_type message);

    std::pair<room_type::iterator, bool> add_client_to_room(const room_id_type &room_id, connection_type handle);
    void remove_client_from_room(connection_type handle);
    bool close_if_empty(const room_id_type &room_id);

    void http_handler(connection_type handle);

    tls_context_pointer_type tls_init_handler(connection_type handle, const fs::path &keyfile, const fs::path &certfile);

    server_type server_;
    rooms_container_type rooms_;
    client_lookup_type client_mapping_;

    thread_type debug_logger_;
    std::atomic<bool> run_debug_logger_;
    bool insecure_;
  };
} // namespace websocket_server
