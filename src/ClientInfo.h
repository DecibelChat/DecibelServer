#pragma once

#include "uuid.hpp"

#include <string>
#include <tuple>

namespace websocket_server
{
  class ClientInfo
  {
  public:
    using room_id_type        = std::string;
    using client_id_type      = std::string;
    using position_value_type = float;
    using position_type       = std::tuple<position_value_type, position_value_type, position_value_type>;

    ClientInfo();

    void assign_room(const room_id_type &room);
    bool update_position(position_type new_position);
    [[nodiscard]] position_value_type distance(const ClientInfo &other) const;

    [[nodiscard]] const room_id_type &room() const;
    [[nodiscard]] const client_id_type &id() const;
    [[nodiscard]] bool unassigned() const;

  private:
    room_id_type room_;
    client_id_type id_;

    position_type position_;

    static uuid::UUID4 uuid_generator_;
  };
} // namespace websocket_server