#include <Biribit/Client/BiribitEvent.h>

namespace Biribit
{

	Event::~Event() {}
	ErrorEvent::~ErrorEvent() {}
	ServerListEvent::~ServerListEvent() {}
	ConnectionEvent::~ConnectionEvent() {}
	RemoteClientsEvent::~RemoteClientsEvent() {}
	RoomListEvent::~RoomListEvent() {}
	JoinedRoomEvent::~JoinedRoomEvent() {}
	BroadcastEvent::~BroadcastEvent() {}
	EntriesEvent::~EntriesEvent() {}

} //namespace Biribit