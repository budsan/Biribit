#pragma once

#include <Biribit/BiribitConfig.h>

typedef unsigned int brbt_time_t;
typedef unsigned int brbt_id_t;
enum brbt_UNASSIGNED { BRBT_UNASSIGNED_ID = 0 };
enum brbt_ReliabilityBitmask
{
	brbt_Unreliable = 0,
	brbt_Reliable = 1,
	brbt_Ordered = 2,
	brbt_ReliableOrdered = 3
};

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

struct brbt_ServerConnection
{
	brbt_id_t id;
	const char* name;
	unsigned int ping;
};

struct brbt_ServerConnection_array
{
	brbt_ServerConnection* arr;
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

struct brbt_Received
{
	brbt_time_t when;
	brbt_id_t connection;
	brbt_id_t room_id;
	unsigned char slot_id;
	unsigned int data_size;
	const void* data;
};

API_C_EXPORT brbt_id_t brbt_CreateClient();
API_C_EXPORT void brbt_DeleteClient(brbt_id_t client);

API_C_EXPORT void brbt_Connect(brbt_id_t client, const char* addr, unsigned short port, const char* password);
API_C_EXPORT void brbt_Disconnect(brbt_id_t client, brbt_id_t id_con);
API_C_EXPORT void brbt_DisconnectAll(brbt_id_t client);

API_C_EXPORT void brbt_DiscoverOnLan(brbt_id_t client, unsigned short port);
API_C_EXPORT void brbt_ClearDiscoverInfo(brbt_id_t client);
API_C_EXPORT void brbt_RefreshDiscoverInfo(brbt_id_t client);

API_C_EXPORT const brbt_ServerInfo_array brbt_GetDiscoverInfo(brbt_id_t client, unsigned int* revision);
API_C_EXPORT const brbt_ServerConnection_array brbt_GetConnections(brbt_id_t client, unsigned int* revision);
API_C_EXPORT const brbt_RemoteClient_array brbt_GetRemoteClients(brbt_id_t client, brbt_id_t id_conn, unsigned int* revision);

API_C_EXPORT brbt_id_t brbt_GetLocalClientId(brbt_id_t client, brbt_id_t id_conn);
API_C_EXPORT void brbt_SetLocalClientParameters(brbt_id_t client, brbt_id_t id_conn, brbt_ClientParameters parameters);

API_C_EXPORT void brbt_RefreshRooms(brbt_id_t client, brbt_id_t id_conn);
API_C_EXPORT const brbt_Room_array brbt_GetRooms(brbt_id_t client, brbt_id_t id_conn, unsigned int* revision);

API_C_EXPORT void brbt_CreateRoom(brbt_id_t client, brbt_id_t id_conn, unsigned int num_slots);
API_C_EXPORT void brbt_CreateRoomAndJoinSlot(brbt_id_t client, brbt_id_t id_conn, unsigned int num_slots, unsigned int slot_to_join_id);

API_C_EXPORT void brbt_JoinRoom(brbt_id_t client, brbt_id_t id_conn, brbt_id_t room_id);
API_C_EXPORT void brbt_JoinRoomAndSlot(brbt_id_t client, brbt_id_t id_conn, brbt_id_t room_id, unsigned int slot_id);

API_C_EXPORT brbt_id_t brbt_GetJoinedRoomId(brbt_id_t client, brbt_id_t id_conn);
API_C_EXPORT unsigned int brbt_GetJoinedRoomSlot(brbt_id_t client, brbt_id_t id_conn);

API_C_EXPORT void brbt_SendToRoom(brbt_id_t client, brbt_id_t id_con, const void* data, unsigned int size, brbt_ReliabilityBitmask mask);

API_C_EXPORT unsigned int brbt_GetDataSizeOfNextReceived(brbt_id_t client);
API_C_EXPORT const brbt_Received* brbt_PullReceived(brbt_id_t client);