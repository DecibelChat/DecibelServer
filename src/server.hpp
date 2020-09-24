#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <thread>
#include <unordered_map>

namespace websocket_server
{
  namespace fs = std::filesystem;

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
    WSS(const Parameters &params);
    ~WSS();

    void start();

  private:
    using server_type              = websocketpp::server<websocketpp::config::asio_tls>;
    using connection_type          = websocketpp::connection_hdl;
    using connection_comparator    = std::owner_less<connection_type>;
    using tls_context_type         = websocketpp::lib::asio::ssl::context;
    using tls_context_pointer_type = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;
    using message_type             = websocketpp::config::asio::message_type;
    using message_pointer_type     = message_type::ptr;

    using room_id_type         = std::string;
    using room_type            = std::set<connection_type, connection_comparator>;
    using rooms_container_type = std::unordered_map<room_id_type, room_type>;
    using client_lookup_type   = std::map<connection_type, room_id_type, connection_comparator>;

    void message_handler(connection_type handle, message_pointer_type message);

    std::pair<room_type::iterator, bool> add_client_to_room(const room_id_type &room_id, connection_type handle);
    void remove_client_from_room(connection_type handle);
    bool close_if_empty(const room_id_type &room_id);

    void http_handler(connection_type handle);

    tls_context_pointer_type tls_init_handler(connection_type handle, const fs::path &keyfile, const fs::path &certfile);

    server_type server_;
    rooms_container_type rooms_;
    client_lookup_type client_mapping_;

    std::jthread debug_logger_;
    std::atomic<bool> run_debug_logger_;
  };
} // namespace websocket_server
