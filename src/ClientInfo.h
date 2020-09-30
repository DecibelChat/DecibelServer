#pragma once

#include "uuid.hpp"

#include <string>

namespace websocket_server
{
  class ClientInfo
  {
  public:
    using room_id_type   = std::string;
    using client_id_type = std::string;

    ClientInfo();

    void assign_room(const room_id_type &room);

    const room_id_type &room() const;
    const client_id_type &id() const;
    bool unassigned() const;

  private:
    room_id_type room_;
    client_id_type id_;

    static uuid::UUID4 uuid_generator_;
  };
} // namespace websocket_server