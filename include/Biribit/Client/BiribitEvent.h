#pragma once

#include <Biribit/Client/BiribitTypes.h>
#include <Biribit/Client/BiribitError.h>

namespace Biribit
{
	enum EventType
	{
		TYPE_ERROR,
		TYPE_SERVER_LIST_UPDATED,
		TYPE_CONNECTIONS_UPDATED,
		TYPE_REMOTE_CLIENTS_UPDATED,
		TYPE_ROOM_LIST_UPDATED,
		TYPE_JOINED_ROOM_UPDATED,
		TYPE_BROADCAST_RECEIVED,
		TYPE_ENTRIES_UPDATED,
	};

	struct API_EXPORT Event
	{
		EventType type;

		virtual ~Event();
	};

	struct API_EXPORT ErrorEvent : public Event
	{
		// ID_ERROR_CODE

		ErrorTypes which;

		virtual ~ErrorEvent();
	};

	struct API_EXPORT ServerListEvent : public Event
	{
		// ID_SERVER_INFO_RESPONSE

		virtual ~ServerListEvent();
	};

	struct API_EXPORT ConnectionEvent : public Event
	{
		// ID_CLIENT_SELF_STATUS

		virtual ~ConnectionEvent();
	};

	struct API_EXPORT RemoteClientsEvent : public Event
	{
		// ID_SERVER_STATUS_RESPONSE
		// ID_CLIENT_STATUS_UPDATED
		// ID_CLIENT_DISCONNECTED

		Connection::id_t connection;

		virtual ~RemoteClientsEvent();
	};

	struct API_EXPORT RoomListEvent : public Event
	{
		// ID_ROOM_LIST_RESPONSE

		Connection::id_t connection;

		virtual ~RoomListEvent();
	};

	struct API_EXPORT JoinedRoomEvent : public Event
	{
		// ID_ROOM_STATUS
		// ID_ROOM_JOIN_RESPONSE

		Connection::id_t connection;

		virtual ~JoinedRoomEvent();
	};

	struct API_EXPORT BroadcastEvent : public Event
	{
		// ID_BROADCAST_FROM_ROOM

		Connection::id_t connection;

		virtual ~BroadcastEvent();
	};

	struct API_EXPORT EntriesEvent : public Event
	{
		// ID_JOURNAL_ENTRIES_STATUS

		Connection::id_t connection;

		virtual ~EntriesEvent();
	};
}