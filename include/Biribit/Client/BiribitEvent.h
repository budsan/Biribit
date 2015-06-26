#pragma once

#include <Biribit/Client/BiribitTypes.h>
#include <Biribit/Client/BiribitError.h>

namespace Biribit
{
	enum EventType
	{
		TYPE_ERROR,
		TYPE_SERVER_LIST,
		TYPE_CONNECTION,
		TYPE_REMOTE_CLIENTS,
		TYPE_ROOM_LIST,
		TYPE_JOINED_ROOM,
		TYPE_BROADCAST,
		TYPE_ENTRIES,
	};

	struct API_EXPORT Event
	{
		EventType type;

		Event(EventType);
		virtual ~Event();
	};

	struct API_EXPORT ErrorEvent : public Event
	{
		ErrorType which;

		ErrorEvent();
		virtual ~ErrorEvent();
	};

	struct API_EXPORT ServerListEvent : public Event
	{
		// ID_SERVER_INFO_RESPONSE

		ServerListEvent();
		virtual ~ServerListEvent();
	};

	struct API_EXPORT ConnectionEvent : public Event
	{
		// ID_CLIENT_SELF_STATUS
		// when DisconnectFrom is called

		ConnectionEvent();
		virtual ~ConnectionEvent();
	};

	struct API_EXPORT RemoteClientsEvent : public Event
	{
		// ID_SERVER_STATUS_RESPONSE
		// ID_CLIENT_STATUS_UPDATED
		// ID_CLIENT_DISCONNECTED

		Connection::id_t connection;

		RemoteClientsEvent();
		virtual ~RemoteClientsEvent();
	};

	struct API_EXPORT RoomListEvent : public Event
	{
		// ID_ROOM_LIST_RESPONSE

		Connection::id_t connection;

		RoomListEvent();
		virtual ~RoomListEvent();
	};

	struct API_EXPORT JoinedRoomEvent : public Event
	{
		// ID_ROOM_STATUS
		// ID_ROOM_JOIN_RESPONSE

		Connection::id_t connection;

		JoinedRoomEvent();
		virtual ~JoinedRoomEvent();
	};

	struct API_EXPORT BroadcastEvent : public Event
	{
		Connection::id_t connection;
		Milliseconds when;
		Room::id_t room_id;
		std::uint8_t slot_id;
		Packet data;

		BroadcastEvent();
		virtual ~BroadcastEvent();
	};

	struct API_EXPORT EntriesEvent : public Event
	{
		// ID_JOURNAL_ENTRIES_STATUS

		Connection::id_t connection;

		EntriesEvent();
		virtual ~EntriesEvent();
	};
}