#pragma once

#include <Biribit/Client/BiribitTypes.h>
#include <Biribit/Client/BiribitError.h>

namespace Biribit
{
	enum EventId
	{
		EVENT_NONE_ID = 0,
		EVENT_ERROR_ID,
		EVENT_SERVER_LIST_ID,
		EVENT_CONNECTION_ID,
		EVENT_SERVER_STATUS_ID,
		EVENT_REMOTE_CLIENT_ID,
		EVENT_ROOM_LIST_ID,
		EVENT_JOINED_ROOM_ID,
		EVENT_BROADCAST_ID,
		EVENT_ENTRIES_ID,
	};

	struct API_EXPORT Event
	{
		EventId id;

		Event();
		Event(EventId);
		virtual ~Event();
	};

	struct API_EXPORT ErrorEvent : public Event
	{
		enum { EVENT_ID = EVENT_ERROR_ID };

		ErrorId which;

		ErrorEvent();
		virtual ~ErrorEvent();
	};

	struct API_EXPORT ServerListEvent : public Event
	{
		enum { EVENT_ID = EVENT_SERVER_LIST_ID };

		ServerListEvent();
		virtual ~ServerListEvent();
	};

	struct API_EXPORT ConnectionEvent : public Event
	{
		enum { EVENT_ID = EVENT_CONNECTION_ID };
		enum EventType
		{
			TYPE_NEW_CONNECTION,
			TYPE_DISCONNECTION,
			TYPE_NAME_UPDATED
		};

		EventType type;
		Connection connection;

		ConnectionEvent();
		virtual ~ConnectionEvent();
	};

	struct API_EXPORT ServerStatusEvent : public Event
	{
		enum { EVENT_ID = EVENT_SERVER_STATUS_ID };

		Connection::id_t connection;
		std::vector<RemoteClient> clients;

		ServerStatusEvent();
		virtual ~ServerStatusEvent();
	};

	struct API_EXPORT RemoteClientEvent : public Event
	{
		enum { EVENT_ID = EVENT_REMOTE_CLIENT_ID };
		enum EventType
		{
			TYPE_CLIENT_UPDATED,
			TYPE_CLIENT_DISCONNECTED
		};

		EventType type;
		Connection::id_t connection;
		RemoteClient client;
		bool self;

		RemoteClientEvent();
		virtual ~RemoteClientEvent();
	};

	struct API_EXPORT RoomListEvent : public Event
	{
		enum { EVENT_ID = EVENT_ROOM_LIST_ID };

		Connection::id_t connection;
		std::vector<Room> rooms;

		RoomListEvent();
		virtual ~RoomListEvent();
	};

	struct API_EXPORT JoinedRoomEvent : public Event
	{
		enum { EVENT_ID = EVENT_JOINED_ROOM_ID };

		Connection::id_t connection;
		Room::id_t room_id;
		std::uint8_t slot_id;

		JoinedRoomEvent();
		virtual ~JoinedRoomEvent();
	};

	struct API_EXPORT BroadcastEvent : public Event
	{
		enum { EVENT_ID = EVENT_BROADCAST_ID };

		Connection::id_t connection;
		milliseconds_t when;
		Room::id_t room_id;
		std::uint8_t slot_id;
		Packet data;

		BroadcastEvent();
		virtual ~BroadcastEvent();
	};

	struct API_EXPORT EntriesEvent : public Event
	{
		enum { EVENT_ID = EVENT_ENTRIES_ID };

		Connection::id_t connection;
		Room::id_t room_id;

		EntriesEvent();
		virtual ~EntriesEvent();
	};
}