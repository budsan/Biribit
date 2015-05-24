#include <Biribit/Unity/BiribitForUnity.h>
#include <Biribit/Common/PrintLog.h>
#include <Biribit/Common/Generic.h>
#include <Biribit/Client.h>

#include <algorithm>
#include <memory>
#include <cstdarg>
#include <chrono>

struct brbt_array
{
	unsigned char* arr;
	unsigned int size;
	unsigned int revision;
	Generic::TempBuffer buffer;

	brbt_array()
		: arr(NULL)
		, size(0)
		, revision(0)
	{}

	bool revision_check(unsigned int new_revision)
	{
		if (revision == new_revision)
			return false;

		revision = new_revision;
		return true;
	}

	template <class T> T* resize(std::size_t count)
	{
		buffer.Ensure(count * sizeof(T));
		arr = (unsigned char*) buffer.data;
		size = count;
		return (T*) arr;
	}

	template <class T> T convert()
	{
		T result;
		result.arr = (decltype(result.arr))arr;
		result.size = size;
		return result;
	}
};

brbt_Client brbt_CreateClient()
{
	brbt_Client client{ (void*) new Biribit::Client() };
	return client;
}

void brbt_DeleteClient(brbt_Client client)
{
	delete (Biribit::Client*) client.impl;
}

void brbt_Connect(brbt_Client client, const char* addr, unsigned short port, const char* password)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->Connect(addr, port, password);
}

void brbt_Disconnect(brbt_Client client, brbt_id_t id_con)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->Disconnect(id_con);
}

void brbt_DisconnectAll(brbt_Client client)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->Disconnect();
}

void brbt_DiscoverOnLan(brbt_Client client, unsigned short port)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->DiscoverOnLan(port);
}

void brbt_ClearDiscoverInfo(brbt_Client client)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->ClearDiscoverInfo();
}

void brbt_RefreshDiscoverInfo(brbt_Client client)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->RefreshDiscoverInfo();
}

const brbt_ServerInfo_array brbt_GetDiscoverInfo(brbt_Client client, unsigned int* revision)
{
	static brbt_array temp_array;
	Biribit::Client* cl = (Biribit::Client*) client.impl;

	unsigned int rev;
	auto client_result = cl->GetDiscoverInfo(&rev);
	static std::vector<std::string> temp_alloc;
	if (temp_array.revision_check(rev))
	{
		temp_alloc.clear();
		brbt_ServerInfo* arr = temp_array.resize<brbt_ServerInfo>(client_result.size());
		for (std::size_t i = 0; i < client_result.size(); i++)
		{
			temp_alloc.push_back(client_result[i].name);
			arr[i].name = temp_alloc.back().c_str();
			temp_alloc.push_back(client_result[i].addr);
			arr[i].addr = temp_alloc.back().c_str();
			arr[i].ping = client_result[i].ping;
			arr[i].port = client_result[i].port;
			arr[i].passwordProtected = client_result[i].passwordProtected;
		}
	}

	if (revision != nullptr)
		*revision = rev;

	return temp_array.convert<brbt_ServerInfo_array>();
}

const brbt_ServerConnection_array brbt_GetConnections(brbt_Client client, unsigned int* revision)
{
	static brbt_array temp_array;
	Biribit::Client* cl = (Biribit::Client*) client.impl;

	unsigned int rev;
	auto client_result = cl->GetConnections(&rev);
	static std::vector<std::string> temp_alloc;
	if (temp_array.revision_check(rev))
	{
		temp_alloc.clear();
		brbt_ServerConnection* arr = temp_array.resize<brbt_ServerConnection>(client_result.size());
		for (std::size_t i = 0; i < client_result.size(); i++)
		{
			arr[i].id = client_result[i].id;
			temp_alloc.push_back(client_result[i].name);
			arr[i].name = temp_alloc.back().c_str();
			arr[i].ping = client_result[i].ping;
		}
	}

	if (revision != nullptr)
		*revision = rev;

	return temp_array.convert<brbt_ServerConnection_array>();
}
const brbt_RemoteClient_array brbt_GetRemoteClients(brbt_Client client, brbt_id_t id_conn, unsigned int* revision)
{
	static brbt_array temp_array;
	Biribit::Client* cl = (Biribit::Client*) client.impl;

	unsigned int rev;
	auto client_result = cl->GetRemoteClients(id_conn, &rev);
	static std::vector<std::string> temp_alloc;
	if (temp_array.revision_check(rev))
	{
		temp_alloc.clear();
		brbt_RemoteClient* arr = temp_array.resize<brbt_RemoteClient>(client_result.size());
		for (std::size_t i = 0; i < client_result.size(); i++)
		{
			arr[i].id = client_result[i].id;
			temp_alloc.push_back(client_result[i].name);
			arr[i].name = temp_alloc.back().c_str();
			temp_alloc.push_back(client_result[i].appid);
			arr[i].appid = temp_alloc.back().c_str();
		}
	}

	if (revision != nullptr)
		*revision = rev;

	return temp_array.convert<brbt_RemoteClient_array>();
}

brbt_id_t brbt_GetLocalClientId(brbt_Client client, brbt_id_t id_conn)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	return cl->GetLocalClientId(id_conn);
}

void brbt_SetLocalClientParameters(brbt_Client client, brbt_id_t id_conn, brbt_ClientParameters _parameters)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	Biribit::ClientParameters parameters;
	parameters.name = _parameters.name;
	parameters.appid = _parameters.appid;
	cl->SetLocalClientParameters(id_conn, parameters);
}

void brbt_RefreshRooms(brbt_Client client, brbt_id_t id_conn)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->RefreshRooms(id_conn);
}

const brbt_Room_array brbt_GetRooms(brbt_Client client, brbt_id_t id_conn, unsigned int* revision)
{
	static brbt_array temp_array;
	Biribit::Client* cl = (Biribit::Client*) client.impl;

	unsigned int rev;
	auto client_result = cl->GetRooms(id_conn, &rev);
	if (temp_array.revision_check(rev))
	{
		brbt_Room* arr = temp_array.resize<brbt_Room>(client_result.size());
		for (std::size_t i = 0; i < client_result.size(); i++)
		{
			arr[i].id = client_result[i].id;
			arr[i].slots_size = client_result[i].slots.size();
			arr[i].slots = client_result[i].slots.data();
		}
	}

	if (revision != nullptr)
		*revision = rev;

	return temp_array.convert<brbt_Room_array>();
}

void brbt_CreateRoom(brbt_Client client, brbt_id_t id_conn, unsigned int num_slots)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->CreateRoom(id_conn, num_slots);
}

void brbt_CreateRoomAndJoinSlot(brbt_Client client, brbt_id_t id_conn, unsigned int num_slots, unsigned int slot_to_join_id)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->CreateRoom(id_conn, num_slots, slot_to_join_id);
}

void brbt_JoinRoom(brbt_Client client, brbt_id_t id_conn, brbt_id_t room_id)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->JoinRoom(id_conn, room_id);
}

void brbt_JoinRoomAndSlot(brbt_Client client, brbt_id_t id_conn, brbt_id_t room_id, unsigned int slot_id)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->JoinRoom(id_conn, room_id, slot_id);
}

brbt_id_t brbt_GetJoinedRoomId(brbt_Client client, brbt_id_t id_conn)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	return cl->GetJoinedRoomId(id_conn);
}

unsigned int brbt_GetJoinedRoomSlot(brbt_Client client, brbt_id_t id_conn)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	return cl->GetJoinedRoomSlot(id_conn);
}

void brbt_SendToRoom(brbt_Client client, brbt_id_t id_con, const void* data, unsigned int size, brbt_ReliabilityBitmask mask)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	cl->SendToRoom(id_con, (const char*) data, size, (Biribit::Packet::ReliabilityBitmask) mask);
}

unsigned int brbt_GetDataSizeOfNextReceived(brbt_Client client)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;
	return cl->GetDataSizeOfNextReceived();
}

const brbt_Received* brbt_PullReceived(brbt_Client client)
{
	Biribit::Client* cl = (Biribit::Client*) client.impl;

	static brbt_Received st_recv;
	static std::unique_ptr<Biribit::Received> recv = cl->PullReceived();

	st_recv.when = recv->when;
	st_recv.connection = recv->connection;
	st_recv.room_id = recv->room_id;
	st_recv.slot_id = recv->slot_id;
	st_recv.data_size = recv->data.getDataSize();
	st_recv.data = (const char*) recv->data.getData();

	return &st_recv;
}
