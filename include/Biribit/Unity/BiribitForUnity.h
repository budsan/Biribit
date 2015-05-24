#pragma once

#include <Biribit/BiribitConfig.h>

struct brbt_Client
{
	void* impl;
};

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

API_C_EXPORT brbt_Client brbt_CreateClient();
API_C_EXPORT void brbt_DeleteClient(brbt_Client);

API_C_EXPORT void brbt_Connect(brbt_Client client, const char* addr, unsigned short port, const char* password);
API_C_EXPORT void brbt_Disconnect(brbt_Client client, brbt_id_t id_con);
API_C_EXPORT void brbt_DisconnectAll(brbt_Client client);

API_C_EXPORT void brbt_DiscoverOnLan(brbt_Client client, unsigned short port);
API_C_EXPORT void brbt_ClearDiscoverInfo(brbt_Client client);
API_C_EXPORT void brbt_RefreshDiscoverInfo(brbt_Client client);

API_C_EXPORT const brbt_ServerInfo_array brbt_GetDiscoverInfo(brbt_Client client, unsigned int* revision);
API_C_EXPORT const brbt_ServerConnection_array brbt_GetConnections(brbt_Client client, unsigned int* revision);
API_C_EXPORT const brbt_RemoteClient_array brbt_GetRemoteClients(brbt_Client client, brbt_id_t id_conn, unsigned int* revision);

API_C_EXPORT brbt_id_t brbt_GetLocalClientId(brbt_Client client, brbt_id_t id_conn);
API_C_EXPORT void brbt_SetLocalClientParameters(brbt_Client client, brbt_id_t id_conn, brbt_ClientParameters parameters);

API_C_EXPORT void brbt_RefreshRooms(brbt_Client client, brbt_id_t id_conn);
API_C_EXPORT const brbt_Room_array brbt_GetRooms(brbt_Client client, brbt_id_t id_conn, unsigned int* revision);

API_C_EXPORT void brbt_CreateRoom(brbt_Client client, brbt_id_t id_conn, unsigned int num_slots);
API_C_EXPORT void brbt_CreateRoomAndJoinSlot(brbt_Client client, brbt_id_t id_conn, unsigned int num_slots, unsigned int slot_to_join_id);

API_C_EXPORT void brbt_JoinRoom(brbt_Client client, brbt_id_t id_conn, brbt_id_t room_id);
API_C_EXPORT void brbt_JoinRoomAndSlot(brbt_Client client, brbt_id_t id_conn, brbt_id_t room_id, unsigned int slot_id);

API_C_EXPORT brbt_id_t brbt_GetJoinedRoomId(brbt_Client client, brbt_id_t id_conn);
API_C_EXPORT unsigned int brbt_GetJoinedRoomSlot(brbt_Client client, brbt_id_t id_conn);

API_C_EXPORT void brbt_SendToRoom(brbt_Client client, brbt_id_t id_con, const void* data, unsigned int size, brbt_ReliabilityBitmask mask);

API_C_EXPORT unsigned int brbt_GetDataSizeOfNextReceived(brbt_Client client);
API_C_EXPORT const brbt_Received* brbt_PullReceived(brbt_Client client);