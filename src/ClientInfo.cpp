#include "ClientInfo.h"

namespace websocket_server
{
  constexpr auto empty_room = "";

  uuid::UUID4 ClientInfo::uuid_generator_;

  ClientInfo::ClientInfo() : room_(empty_room), id_(uuid_generator_())
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
} // namespace websocket_server