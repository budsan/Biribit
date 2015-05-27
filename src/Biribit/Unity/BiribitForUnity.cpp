#include <Biribit/Unity/BiribitForUnity.h>
#include <Biribit/Client/BiribitClientExports.h>
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

std::vector<std::unique_ptr<Biribit::Client>> clients(1);

brbt_id_t brbt_CreateClient()
{
	brbt_id_t client;
	for (client = 1; client < clients.size() && clients[client] != nullptr; client++);
	if (client == clients.size())
		clients.resize(client + 1);

	clients[client] = std::unique_ptr<Biribit::Client>(new Biribit::Client());
	return client;
}

// Useful in Unity for changing between scenes and keeping same instance.
brbt_id_t single_instance = 0;
brbt_id_t brbt_GetClientInstance()
{
	if (single_instance == 0)
		single_instance = brbt_CreateClient();

	return single_instance;
}

void brbt_DeleteClient(brbt_id_t client)
{
	if (client > 0 && client < clients.size())
	{
		if (single_instance == client)
			single_instance = 0;

		clients[client] = nullptr;
	}
}

void brbt_Connect(brbt_id_t client, const char* addr, unsigned short port, const char* password)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->Connect(addr, port, password);
}

void brbt_Disconnect(brbt_id_t client, brbt_id_t id_con)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->Disconnect(id_con);
}

void brbt_DisconnectAll(brbt_id_t client)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->Disconnect();
}

void brbt_DiscoverOnLan(brbt_id_t client, unsigned short port)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->DiscoverOnLan(port);
}

void brbt_ClearDiscoverInfo(brbt_id_t client)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->ClearDiscoverInfo();
}

void brbt_RefreshDiscoverInfo(brbt_id_t client)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->RefreshDiscoverInfo();
}

const brbt_ServerInfo_array brbt_GetDiscoverInfo(brbt_id_t client, unsigned int* revision)
{
	static brbt_array temp_array;
	std::unique_ptr<Biribit::Client>& cl = clients[client];

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

const brbt_ServerConnection_array brbt_GetConnections(brbt_id_t client, unsigned int* revision)
{
	static brbt_array temp_array;
	std::unique_ptr<Biribit::Client>& cl = clients[client];

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

const brbt_RemoteClient_array brbt_GetRemoteClients(brbt_id_t client, brbt_id_t id_conn, unsigned int* revision)
{
	static brbt_array temp_array;
	std::unique_ptr<Biribit::Client>& cl = clients[client];

	unsigned int rev;
	auto client_result = cl->GetRemoteClients(id_conn, &rev);
	static std::vector<std::string> remoteclients_temp_alloc;
	if (temp_array.revision_check(rev))
	{
		remoteclients_temp_alloc.clear();
		brbt_RemoteClient* arr = temp_array.resize<brbt_RemoteClient>(client_result.size());
		for (std::size_t i = 0; i < client_result.size(); i++)
		{
			arr[i].id = client_result[i].id;
			remoteclients_temp_alloc.push_back(client_result[i].name);
			arr[i].name = remoteclients_temp_alloc.back().c_str();
			remoteclients_temp_alloc.push_back(client_result[i].appid);
			arr[i].appid = remoteclients_temp_alloc.back().c_str();
		}
	}

	if (revision != nullptr)
		*revision = rev;
	return temp_array.convert<brbt_RemoteClient_array>();
}

brbt_id_t brbt_GetLocalClientId(brbt_id_t client, brbt_id_t id_conn)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	return cl->GetLocalClientId(id_conn);
}

void brbt_SetLocalClientParameters(brbt_id_t client, brbt_id_t id_conn, brbt_ClientParameters _parameters)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	Biribit::ClientParameters parameters;
	parameters.name = _parameters.name;
	parameters.appid = _parameters.appid;
	cl->SetLocalClientParameters(id_conn, parameters);
}

void brbt_RefreshRooms(brbt_id_t client, brbt_id_t id_conn)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->RefreshRooms(id_conn);
}

const brbt_Room_array brbt_GetRooms(brbt_id_t client, brbt_id_t id_conn, unsigned int* revision)
{
	static brbt_array temp_array;
	std::unique_ptr<Biribit::Client>& cl = clients[client];

	unsigned int rev;
	auto client_result = cl->GetRooms(id_conn, &rev);
	static std::vector<std::vector<unsigned int>> temp_alloc;
	if (temp_array.revision_check(rev))
	{
		temp_alloc.clear();
		brbt_Room* arr = temp_array.resize<brbt_Room>(client_result.size());
		for (std::size_t i = 0; i < client_result.size(); i++)
		{
			arr[i].id = client_result[i].id;
			arr[i].slots_size = client_result[i].slots.size();
			temp_alloc.push_back(client_result[i].slots);
			arr[i].slots = &temp_alloc.back().at(0);
		}
	}

	if (revision != nullptr)
		*revision = rev;
	return temp_array.convert<brbt_Room_array>();
}

void brbt_CreateRoom(brbt_id_t client, brbt_id_t id_conn, unsigned int num_slots)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->CreateRoom(id_conn, num_slots);
}

void brbt_CreateRoomAndJoinSlot(brbt_id_t client, brbt_id_t id_conn, unsigned int num_slots, unsigned int slot_to_join_id)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->CreateRoom(id_conn, num_slots, slot_to_join_id);
}

void brbt_JoinRandomOrCreateRoom(brbt_id_t client, brbt_id_t id_conn, unsigned int num_slots)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->JoinRandomOrCreateRoom(id_conn, num_slots);
}

void brbt_JoinRoom(brbt_id_t client, brbt_id_t id_conn, brbt_id_t room_id)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->JoinRoom(id_conn, room_id);
}

void brbt_JoinRoomAndSlot(brbt_id_t client, brbt_id_t id_conn, brbt_id_t room_id, unsigned int slot_id)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->JoinRoom(id_conn, room_id, slot_id);
}

brbt_id_t brbt_GetJoinedRoomId(brbt_id_t client, brbt_id_t id_conn)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	return cl->GetJoinedRoomId(id_conn);
}

unsigned int brbt_GetJoinedRoomSlot(brbt_id_t client, brbt_id_t id_conn)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	return cl->GetJoinedRoomSlot(id_conn);
}

void brbt_SendToRoom(brbt_id_t client, brbt_id_t id_con, const void* data, unsigned int size, brbt_ReliabilityBitmask mask)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	cl->SendToRoom(id_con, (const char*) data, size, (Biribit::Packet::ReliabilityBitmask) mask);
}

unsigned int brbt_GetDataSizeOfNextReceived(brbt_id_t client)
{
	std::unique_ptr<Biribit::Client>& cl = clients[client];
	return cl->GetDataSizeOfNextReceived();
}

const brbt_Received* brbt_PullReceived(brbt_id_t client)
{
	static std::unique_ptr<Biribit::Received> recv;
	static brbt_Received st_recv;

	std::unique_ptr<Biribit::Client>& cl = clients[client];
	recv = cl->PullReceived();
	if (recv != nullptr)
	{
		st_recv.when = recv->when;
		st_recv.connection = recv->connection;
		st_recv.room_id = recv->room_id;
		st_recv.slot_id = recv->slot_id;
		st_recv.data_size = recv->data.getDataSize();
		st_recv.data = (const char*)recv->data.getData();
		return &st_recv;
	}
	else
	{
		return nullptr;
	}	
}
