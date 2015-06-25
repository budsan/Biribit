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

struct brbt_context
{
	Biribit::Client client;

	brbt_array GetServerList_array;
	std::vector<std::string> GetServerList_alloc;

	brbt_array GetConnections_array;
	std::vector<std::string> GetConnections_alloc;

	brbt_array GetRemoteClients_array;
	std::vector<std::string> GetRemoteClients_alloc;

	brbt_array GetRooms_array;
	std::vector<std::vector<unsigned int>> GetRooms_alloc;

	std::unique_ptr<Biribit::Received> recv;
	brbt_Received m_recv;
	brbt_Entry m_entry;
};

// Useful in Unity for changing between scenes and keeping same instance.
brbt_Client single_instance = NULL;

brbt_Client brbt_CreateClient()
{
	brbt_Client client;
	client = (void*) new brbt_context();
	return client;
}

brbt_Client brbt_GetClientInstance()
{
	if (single_instance == NULL)
		single_instance = brbt_CreateClient();

	return single_instance;
}

void brbt_DeleteClient(brbt_Client client)
{
	if (single_instance == client)
		single_instance = NULL;

	brbt_context* context = (brbt_context*) client;
	delete context;
}

void brbt_Connect(brbt_Client client, const char* addr, unsigned short port, const char* password)
{
	brbt_context* context = (brbt_context*) client;
	Biribit::Client* cl = &(context->client);
	cl->Connect(addr, port, password);
}

void brbt_Disconnect(brbt_Client client, brbt_id_t id_con)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->Disconnect(id_con);
}

void brbt_DisconnectAll(brbt_Client client)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->Disconnect();
}

void brbt_DiscoverServersOnLAN(brbt_Client client, unsigned short port)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->DiscoverServersOnLAN(port);
}

void brbt_ClearServerList(brbt_Client client)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->ClearServerList();
}

void brbt_RefreshServerList(brbt_Client client)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->RefreshServerList();
}

const brbt_ServerInfo_array brbt_GetServerList(brbt_Client client, unsigned int* revision)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	brbt_array& temp_array = context->GetServerList_array;
	std::vector<std::string>& temp_alloc = context->GetServerList_alloc;

	unsigned int rev;
	auto client_result = cl->GetServerList(&rev);
	if (temp_array.revision_check(rev))
	{
		temp_alloc.clear();
		brbt_ServerInfo* arr = temp_array.resize<brbt_ServerInfo>(client_result.size());

		// Alloc everything first. Pointers may change if vector reallocates.
		for (std::size_t i = 0; i < client_result.size(); i++) {
			std::size_t name_i = temp_alloc.size(); temp_alloc.push_back(client_result[i].name);
			std::size_t addr_i = temp_alloc.size(); temp_alloc.push_back(client_result[i].addr);
		}

		std::size_t push_i = 0;
		for (std::size_t i = 0; i < client_result.size(); i++)
		{
			arr[i].name = temp_alloc[push_i++].c_str();
			arr[i].addr = temp_alloc[push_i++].c_str();
			arr[i].ping = client_result[i].ping;
			arr[i].port = client_result[i].port;
			arr[i].passwordProtected = client_result[i].passwordProtected;
		}
	}

	if (revision != nullptr)
		*revision = rev;
	return temp_array.convert<brbt_ServerInfo_array>();
}

const brbt_Connection_array brbt_GetConnections(brbt_Client client, unsigned int* revision)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	brbt_array& temp_array = context->GetConnections_array;
	std::vector<std::string>& temp_alloc = context->GetConnections_alloc;

	unsigned int rev;
	auto client_result = cl->GetConnections(&rev);
	if (temp_array.revision_check(rev))
	{
		temp_alloc.clear();
		brbt_Connection* arr = temp_array.resize<brbt_Connection>(client_result.size());

		// Alloc everything first. Pointers may change if vector reallocates.
		for (std::size_t i = 0; i < client_result.size(); i++)
			temp_alloc.push_back(client_result[i].name);

		for (std::size_t i = 0; i < client_result.size(); i++)
		{
			arr[i].id = client_result[i].id;
			arr[i].name = temp_alloc[i].c_str();
			arr[i].ping = client_result[i].ping;
		}
	}

	if (revision != nullptr)
		*revision = rev;
	return temp_array.convert<brbt_Connection_array>();
}

const brbt_RemoteClient_array brbt_GetRemoteClients(brbt_Client client, brbt_id_t id_conn, unsigned int* revision)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	brbt_array& temp_array = context->GetRemoteClients_array;
	std::vector<std::string>& temp_alloc = context->GetRemoteClients_alloc;

	unsigned int rev;
	auto client_result = cl->GetRemoteClients(id_conn, &rev);
	if (temp_array.revision_check(rev))
	{
		temp_alloc.clear();
		brbt_RemoteClient* arr = temp_array.resize<brbt_RemoteClient>(client_result.size());

		// Alloc everything first. Pointers may change if vector reallocates.
		for (std::size_t i = 0; i < client_result.size(); i++) {
			temp_alloc.push_back(client_result[i].name);
			temp_alloc.push_back(client_result[i].appid);
		}

		std::size_t push_i = 0;
		for (std::size_t i = 0; i < client_result.size(); i++)
		{
			arr[i].id = client_result[i].id;
			arr[i].name = temp_alloc[push_i++].c_str();
			arr[i].appid = temp_alloc[push_i++].c_str();
		}
	}

	if (revision != nullptr)
		*revision = rev;
	return temp_array.convert<brbt_RemoteClient_array>();
}

brbt_id_t brbt_GetLocalClientId(brbt_Client client, brbt_id_t id_conn)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	return cl->GetLocalClientId(id_conn);
}

void brbt_SetLocalClientParameters(brbt_Client client, brbt_id_t id_conn, brbt_ClientParameters _parameters)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	Biribit::ClientParameters parameters;
	parameters.name = _parameters.name;
	parameters.appid = _parameters.appid;
	cl->SetLocalClientParameters(id_conn, parameters);
}

void brbt_RefreshRooms(brbt_Client client, brbt_id_t id_conn)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->RefreshRooms(id_conn);
}

const brbt_Room_array brbt_GetRooms(brbt_Client client, brbt_id_t id_conn, unsigned int* revision)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	brbt_array& temp_array = context->GetRooms_array;
	std::vector<std::vector<unsigned int>>& temp_alloc = context->GetRooms_alloc;

	unsigned int rev;
	auto client_result = cl->GetRooms(id_conn, &rev);
	if (temp_array.revision_check(rev))
	{
		temp_alloc.clear();
		brbt_Room* arr = temp_array.resize<brbt_Room>(client_result.size());

		// Alloc everything first. Pointers may change if vector reallocates.
		for (std::size_t i = 0; i < client_result.size(); i++)
			temp_alloc.push_back(client_result[i].slots);

		for (std::size_t i = 0; i < client_result.size(); i++) {
			arr[i].id = client_result[i].id;
			arr[i].slots_size = temp_alloc[i].size();
			arr[i].slots = &(temp_alloc[i].at(0));
		}
	}

	if (revision != nullptr)
		*revision = rev;
	return temp_array.convert<brbt_Room_array>();
}

void brbt_CreateRoom(brbt_Client client, brbt_id_t id_conn, unsigned int num_slots)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->CreateRoom(id_conn, num_slots);
}

void brbt_CreateRoomAndJoinSlot(brbt_Client client, brbt_id_t id_conn, unsigned int num_slots, unsigned int slot_to_join_id)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->CreateRoom(id_conn, num_slots, slot_to_join_id);
}

void brbt_JoinRandomOrCreateRoom(brbt_Client client, brbt_id_t id_conn, unsigned int num_slots)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->JoinRandomOrCreateRoom(id_conn, num_slots);
}

void brbt_JoinRoom(brbt_Client client, brbt_id_t id_conn, brbt_id_t room_id)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->JoinRoom(id_conn, room_id);
}

void brbt_JoinRoomAndSlot(brbt_Client client, brbt_id_t id_conn, brbt_id_t room_id, unsigned int slot_id)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->JoinRoom(id_conn, room_id, slot_id);
}

brbt_id_t brbt_GetJoinedRoomId(brbt_Client client, brbt_id_t id_conn)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	return cl->GetJoinedRoomId(id_conn);
}

unsigned int brbt_GetJoinedRoomSlot(brbt_Client client, brbt_id_t id_conn)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	return cl->GetJoinedRoomSlot(id_conn);
}

void brbt_SendBroadcast(brbt_Client client, brbt_id_t id_con, const void* data, unsigned int size, brbt_ReliabilityBitmask mask)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->SendBroadcast(id_con, (const char*) data, size, (Biribit::Packet::ReliabilityBitmask) mask);
}

void brbt_SendEntry(brbt_Client client, brbt_id_t id_con, const void* data, unsigned int size)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->SendEntry(id_con, (const char*)data, size);
}

unsigned int brbt_GetDataSizeOfNextReceived(brbt_Client client)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	return cl->GetDataSizeOfNextReceived();
}

const brbt_Received* brbt_PullReceived(brbt_Client client)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);

	context->recv = cl->PullReceived();
	if (context->recv != nullptr)
	{
		context->m_recv.when = context->recv->when;
		context->m_recv.connection = context->recv->connection;
		context->m_recv.room_id = context->recv->room_id;
		context->m_recv.slot_id = context->recv->slot_id;
		context->m_recv.data_size = context->recv->data.getDataSize();
		context->m_recv.data = context->recv->data.getData();
		return &(context->m_recv);
	}
	else
	{
		return nullptr;
	}	
}

brbt_id_t brbt_GetEntriesCount(brbt_Client client, brbt_id_t id_con)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	return cl->GetEntriesCount(id_con);
}

const brbt_Entry* brbt_GetEntry(brbt_Client client, brbt_id_t id_con, brbt_id_t id_entry)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	const Biribit::Entry& entry = cl->GetEntry(id_con, id_entry);
	if (entry.id != Biribit::Entry::UNASSIGNED_ID)
	{
		context->m_entry.id = entry.id;
		context->m_entry.slot_id = entry.from_slot;
		context->m_entry.data_size = entry.data.getDataSize();
		context->m_entry.data = entry.data.getData();
	}
	else
	{
		context->m_entry.id = Biribit::Entry::UNASSIGNED_ID;
		context->m_entry.slot_id = 0;
		context->m_entry.data_size = 0;
		context->m_entry.data = nullptr;
	}

	return &(context->m_entry);
}
