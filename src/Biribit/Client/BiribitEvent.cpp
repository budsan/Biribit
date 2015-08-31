#include <Biribit/Client/BiribitEvent.h>

namespace Biribit
{

Event::Event() : id(EVENT_NONE_ID) {}
Event::Event(EventId _type) : id(_type) {}
Event::~Event() {}

ErrorEvent::ErrorEvent() : Event(EVENT_ERROR_ID) {}
ErrorEvent::~ErrorEvent() {}

ServerListEvent::ServerListEvent() : Event(EVENT_SERVER_LIST_ID) {}
ServerListEvent::~ServerListEvent() {}

ConnectionEvent::ConnectionEvent() : Event(EVENT_CONNECTION_ID) {}
ConnectionEvent::~ConnectionEvent() {}

ServerStatusEvent::ServerStatusEvent() : Event(EVENT_SERVER_STATUS_ID) {}
ServerStatusEvent::~ServerStatusEvent() {}

RemoteClientEvent::RemoteClientEvent() : Event(EVENT_REMOTE_CLIENT_ID) {}
RemoteClientEvent::~RemoteClientEvent() {}

RoomListEvent::RoomListEvent() : Event(EVENT_ROOM_LIST_ID) {}
RoomListEvent::~RoomListEvent() {}

JoinedRoomEvent::JoinedRoomEvent() : Event(EVENT_JOINED_ROOM_ID) {}
JoinedRoomEvent::~JoinedRoomEvent() {}

BroadcastEvent::BroadcastEvent() : Event(EVENT_BROADCAST_ID) {}
BroadcastEvent::~BroadcastEvent() {}

EntriesEvent::EntriesEvent() : Event(EVENT_ENTRIES_ID) {}
EntriesEvent::~EntriesEvent() {}

} //namespace Biribit