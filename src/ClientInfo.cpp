#include "ClientInfo.h"

#include <cmath>
#include <utility>

namespace websocket_server
{
  constexpr auto empty_room = "";

  uuid::UUID4 ClientInfo::uuid_generator_;

  ClientInfo::ClientInfo() : room_(empty_room), id_(uuid_generator_()), position_{0, 0, 0}
  {
  }

  void ClientInfo::assign_room(const room_id_type &room)
  {
    room_ = room;
  }

  const ClientInfo::room_id_type &ClientInfo::room() const
  {
    return room_;
  }

  const ClientInfo::client_id_type &ClientInfo::id() const
  {
    return id_;
  }

  bool ClientInfo::unassigned() const
  {
    return room_ == empty_room;
  }

  bool ClientInfo::update_position(ClientInfo::position_type new_position)
  {
    return new_position != std::exchange(position_, new_position);
  }

  ClientInfo::position_value_type ClientInfo::distance(const ClientInfo &other) const
  {
    const auto &[x, y, z]                   = position_;
    const auto &[x_other, y_other, z_other] = other.position_;

    return std::hypot(x_other - x, y_other - y, z_other - z);
  }
} // namespace websocket_server