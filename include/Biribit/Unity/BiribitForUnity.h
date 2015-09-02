#pragma once

#include <Biribit/BiribitConfig.h>

typedef unsigned int brbt_time_t;
typedef unsigned int brbt_id_t;
typedef unsigned char brbt_bool;
typedef unsigned char brbt_slot_id_t;

enum brbt_UNASSIGNED { BRBT_UNASSIGNED_ID = 0 };

enum brbt_BoolValues
{
	BRBT_FALSE = 0,
	BRBT_TRUE = 1
};

enum brbt_ReliabilityBitmask
{
	BRBT_UNRELIABLE = 0,
	BRBT_RELIABLE = 1,
	BRBT_ORDERED = 2,
	BRBT_RELIABLEORDERED = 3
};

enum brbt_ErrorId
{
	BRBT_ERROR_CODE = 0,
	BRBT_WARN_CLIENT_NAME_IN_USE,
	BRBT_WARN_CANNOT_LIST_ROOMS_WITHOUT_APPID,
	BRBT_WARN_CANNOT_CREATE_ROOM_WITHOUT_APPID,
	BRBT_WARN_CANNOT_CREATE_ROOM_WITH_WRONG_SLOT_NUMBER,
	BRBT_WARN_CANNOT_CREATE_ROOM_WITH_TOO_MANY_SLOTS,
	BRBT_WARN_CANNOT_JOIN_WITHOUT_ROOM_ID,
	BRBT_WARN_CANNOT_JOIN_TO_UNEXISTING_ROOM,
	BRBT_WARN_CANNOT_JOIN_TO_OTHER_APP_ROOM,
	BRBT_WARN_CANNOT_JOIN_TO_OCCUPIED_SLOT,
	BRBT_WARN_CANNOT_JOIN_TO_INVALID_SLOT,
	BRBT_WARN_CANNOT_JOIN_TO_FULL_ROOM
};

enum brbt_ConnectionEventType
{
	BRBT_TYPE_NEW_CONNECTION,
	BRBT_TYPE_DISCONNECTION,
	BRBT_TYPE_NAME_UPDATED
};

enum brbt_RemoteClientEventType
{
	BRBT_TYPE_CLIENT_UPDATED,
	BRBT_TYPE_CLIENT_DISCONNECTED
};

typedef void* brbt_Client;

struct brbt_ServerInfo
{
	const char* name;
	const char* addr;
	unsigned int ping;
	unsigned short port;
	unsigned char passwordProtected;
};

struct brbt_ServerInfo_array
{
	brbt_ServerInfo* arr;
	unsigned int size;
};

struct brbt_Connection
{
	brbt_id_t id;
	const char* name;
	unsigned int ping;
};

struct brbt_Connection_array
{
	brbt_Connection* arr;
	unsigned int size;
};

struct brbt_RemoteClient
{
	brbt_id_t id;
	const char* name;
	const char* appid;
};

struct brbt_RemoteClient_array
{
	brbt_RemoteClient* arr;
	unsigned int size;
};


struct brbt_ClientParameters
{
	const char* name;
	const char* appid;
};

struct brbt_Room
{
	brbt_id_t id;
	unsigned int slots_size;
	const brbt_id_t* slots;
};

struct brbt_Room_array
{
	brbt_Room* arr;
	unsigned int size;
};

struct brbt_Entry
{
	brbt_id_t id;
	brbt_slot_id_t slot_id;
	unsigned int data_size;
	const void* data;
};

struct brbt_ErrorEvent
{
	brbt_ErrorId which;
};

struct brbt_ServerListEvent
{
	brbt_id_t dummy;
};

struct brbt_ConnectionEvent
{
	brbt_ConnectionEventType type;
	brbt_Connection connection;
};

struct brbt_ServerStatusEvent
{
	brbt_id_t connection;
	brbt_RemoteClient_array clients;
};

struct brbt_RemoteClientEvent
{
	brbt_RemoteClientEventType type;
	brbt_id_t connection;
	brbt_RemoteClient client;
	brbt_bool self;
};

struct brbt_RoomListEvent
{
	brbt_id_t connection;
	brbt_Room_array rooms;
};

struct brbt_JoinedRoomEvent
{
	brbt_id_t connection;
	brbt_id_t room_id;
	brbt_slot_id_t slot_id;
};

struct brbt_BroadcastEvent
{
	brbt_id_t connection;
	brbt_time_t when;
	brbt_id_t room_id;
	brbt_slot_id_t slot_id;
	unsigned int data_size;
	const void* data;
};

struct brbt_EntriesEvent
{
	brbt_id_t connection;
	brbt_id_t room_id;
};

typedef void (STDCALL *brbt_ErrorEvent_callback)(const brbt_ErrorEvent*);
typedef void (STDCALL *brbt_ServerListEvent_callback)(const brbt_ServerListEvent*);
typedef void (STDCALL *brbt_ConnectionEvent_callback)(const brbt_ConnectionEvent*);
typedef void (STDCALL *brbt_ServerStatusEvent_callback)(const brbt_ServerStatusEvent*);
typedef void (STDCALL *brbt_RemoteClientEvent_callback)(const brbt_RemoteClientEvent*);
typedef void (STDCALL *brbt_RoomListEvent_callback)(const brbt_RoomListEvent*);
typedef void (STDCALL *brbt_JoinedRoomEvent_callback)(const brbt_JoinedRoomEvent*);
typedef void (STDCALL *brbt_BroadcastEvent_callback)(const brbt_BroadcastEvent*);
typedef void (STDCALL *brbt_EntriesEvent_callback)(const brbt_EntriesEvent*);

struct brbt_EventCallbackTable
{
	brbt_ErrorEvent_callback error;
	brbt_ServerListEvent_callback server_list;
	brbt_ConnectionEvent_callback connection;
	brbt_ServerStatusEvent_callback server_status;
	brbt_RemoteClientEvent_callback remote_client;
	brbt_RoomListEvent_callback room_list;
	brbt_JoinedRoomEvent_callback joined_room;
	brbt_BroadcastEvent_callback broadcast;
	brbt_EntriesEvent_callback entries;
};

typedef void (STDCALL *brbt_ServerInfo_callback)(const brbt_ServerInfo_array);
typedef void (STDCALL *brbt_Connection_callback)(const brbt_Connection_array);
typedef void (STDCALL *brbt_RemoteClient_callback)(const brbt_RemoteClient_array);
typedef void (STDCALL *brbt_Room_callback)(const brbt_Room_array);

API_C_EXPORT brbt_Client brbt_CreateClient();
API_C_EXPORT brbt_Client brbt_GetClientInstance();
API_C_EXPORT void brbt_DeleteClient(brbt_Client client);

API_C_EXPORT void brbt_Connect(brbt_Client client, const char* addr, unsigned short port, const char* password);
API_C_EXPORT void brbt_Disconnect(brbt_Client client, brbt_id_t id_con);
API_C_EXPORT void brbt_DisconnectAll(brbt_Client client);

API_C_EXPORT void brbt_DiscoverServersOnLAN(brbt_Client client, unsigned short port);
API_C_EXPORT void brbt_ClearServerList(brbt_Client client);
API_C_EXPORT void brbt_RefreshServerList(brbt_Client client);

API_C_EXPORT void brbt_GetServerList(brbt_Client client, brbt_ServerInfo_callback callback);
API_C_EXPORT void brbt_GetConnections(brbt_Client client, brbt_Connection_callback callback);
API_C_EXPORT void brbt_GetRemoteClients(brbt_Client client, brbt_id_t id_conn, brbt_RemoteClient_callback callback);
API_C_EXPORT void brbt_GetRooms(brbt_Client client, brbt_id_t id_conn, brbt_Room_callback callback);

API_C_EXPORT void brbt_UpdateCallbacks(brbt_Client client);

API_C_EXPORT brbt_id_t brbt_GetLocalClientId(brbt_Client client, brbt_id_t id_conn);
API_C_EXPORT void brbt_SetLocalClientParameters(brbt_Client client, brbt_id_t id_conn, brbt_ClientParameters parameters);

API_C_EXPORT void brbt_RefreshRooms(brbt_Client client, brbt_id_t id_conn);

API_C_EXPORT void brbt_CreateRoom(brbt_Client client, brbt_id_t id_conn, brbt_slot_id_t num_slots);
API_C_EXPORT void brbt_CreateRoomAndJoinSlot(brbt_Client client, brbt_id_t id_conn, brbt_slot_id_t num_slots, brbt_slot_id_t slot_to_join_id);

API_C_EXPORT void brbt_JoinRandomOrCreateRoom(brbt_Client client, brbt_id_t id_conn, brbt_slot_id_t num_slots);

API_C_EXPORT void brbt_JoinRoom(brbt_Client client, brbt_id_t id_conn, brbt_id_t room_id);
API_C_EXPORT void brbt_JoinRoomAndSlot(brbt_Client client, brbt_id_t id_conn, brbt_id_t room_id, brbt_slot_id_t slot_id);

API_C_EXPORT brbt_id_t brbt_GetJoinedRoomId(brbt_Client client, brbt_id_t id_conn);
API_C_EXPORT unsigned int brbt_GetJoinedRoomSlot(brbt_Client client, brbt_id_t id_conn);

API_C_EXPORT void brbt_SendBroadcast(brbt_Client client, brbt_id_t id_con, const void* data, unsigned int size, brbt_ReliabilityBitmask mask);
API_C_EXPORT void brbt_SendEntry(brbt_Client client, brbt_id_t id_con, const void* data, unsigned int size);

API_C_EXPORT void brbt_PullEvents(brbt_Client client, const brbt_EventCallbackTable* table);

API_C_EXPORT brbt_id_t brbt_GetEntriesCount(brbt_Client client, brbt_id_t id_con);
API_C_EXPORT const brbt_Entry* brbt_GetEntry(brbt_Client client, brbt_id_t id_con, brbt_id_t id_entry);
