#include "ClientInfo.h"

#include <cmath>

namespace websocket_server
{
  constexpr auto empty_room = "";

  uuid::UUID4 ClientInfo::uuid_generator_;

  ClientInfo::ClientInfo() : room_(empty_room), id_(uuid_generator_()), x_(0), y_(0), z_(0)
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

  bool ClientInfo::update_position(ClientInfo::position_value_type x,
                                   ClientInfo::position_value_type y,
                                   ClientInfo::position_value_type z)
  {
    if (x == x_ && y == y_ && z == z_)
    {
      return false;
    }

    x_ = x;
    y_ = y;
    z_ = z;

    return true;
  }

  ClientInfo::position_value_type ClientInfo::distance(const ClientInfo &other) const
  {
    return std::hypot(other.x_ - x_, other.y_ - y_, other.z_ - z_);
  }
} // namespace websocket_server