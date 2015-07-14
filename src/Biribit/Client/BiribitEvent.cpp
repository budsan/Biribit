#include <Biribit/Client/BiribitEvent.h>

namespace Biribit
{

Event::Event() : type(TYPE_NONE) {}
Event::Event(EventType _type) : type(_type) {}
Event::~Event() {}

ErrorEvent::ErrorEvent() : Event(TYPE_ERROR) {}
ErrorEvent::~ErrorEvent() {}

ServerListEvent::ServerListEvent() : Event(TYPE_SERVER_LIST) {}
ServerListEvent::~ServerListEvent() {}

ConnectionEvent::ConnectionEvent() : Event(TYPE_CONNECTION) {}
ConnectionEvent::~ConnectionEvent() {}

RemoteClientsEvent::RemoteClientsEvent() : Event(TYPE_REMOTE_CLIENTS) {}
RemoteClientsEvent::~RemoteClientsEvent() {}

RoomListEvent::RoomListEvent() : Event(TYPE_ROOM_LIST) {}
RoomListEvent::~RoomListEvent() {}

JoinedRoomEvent::JoinedRoomEvent() : Event(TYPE_JOINED_ROOM) {}
JoinedRoomEvent::~JoinedRoomEvent() {}

BroadcastEvent::BroadcastEvent() : Event(TYPE_BROADCAST) {}
BroadcastEvent::~BroadcastEvent() {}

EntriesEvent::EntriesEvent() : Event(TYPE_ENTRIES) {}
EntriesEvent::~EntriesEvent() {}

} //namespace Biribit