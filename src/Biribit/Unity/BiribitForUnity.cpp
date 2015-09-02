#include <Biribit/Unity/BiribitForUnity.h>
#include <Biribit/Client/BiribitClientExports.h>
#include <Biribit/Common/PrintLog.h>
#include <Biribit/Common/Generic.h>
#include <Biribit/Client.h>

#include <list>
#include <algorithm>
#include <memory>
#include <cstdarg>
#include <chrono>

struct brbt_array
{
	unsigned char* arr;
	unsigned int size;
	Generic::TempBuffer buffer;

	brbt_array()
		: arr(NULL)
		, size(0)
	{}

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

	brbt_Entry m_entry;

	std::list<std::function<bool()>> tasks;
};

//-allocs--------------------------------------------------------------------//

brbt_RemoteClient_array alloc_RemoteClient_array(brbt_context* context, const std::vector<Biribit::RemoteClient>& client_result)
{
	brbt_array& temp_array = context->GetRemoteClients_array;
	std::vector<std::string>& temp_alloc = context->GetRemoteClients_alloc;

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

	return temp_array.convert<brbt_RemoteClient_array>();
}

brbt_Room_array alloc_Room_array(brbt_context* context, const std::vector<Biribit::Room>& client_result)
{
	brbt_array& temp_array = context->GetRooms_array;
	std::vector<std::vector<unsigned int>>& temp_alloc = context->GetRooms_alloc;

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

	return temp_array.convert<brbt_Room_array>();
}

//---------------------------------------------------------------------------//

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

template<class T> bool Future_Then(std::shared_future<T> src, std::function<void()> then)
{
	if (!src.valid())
		return true;

	if (src.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
		then();
		return true;
	}

	return false;
}

void brbt_GetServerList(brbt_Client client, brbt_ServerInfo_callback callback)
{
	brbt_context* context = (brbt_context*)client; Biribit::Client* cl = &(context->client);
	auto future = context->client.GetServerList().share();
	context->tasks.push_back([context, future, callback]() -> bool
	{
		return Future_Then(future, [context, future, callback]
		{
			brbt_array& temp_array = context->GetServerList_array;
			std::vector<std::string>& temp_alloc = context->GetServerList_alloc;
			const std::vector<Biribit::ServerInfo>& client_result = future.get();

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

			callback(temp_array.convert<brbt_ServerInfo_array>());
		});
	});
}

void brbt_GetConnections(brbt_Client client, brbt_Connection_callback callback)
{
	brbt_context* context = (brbt_context*)client; Biribit::Client* cl = &(context->client);
	auto future = context->client.GetConnections().share();
	context->tasks.push_back([context, future, callback]() -> bool
	{
		return Future_Then(future, [context, future, callback]
		{
			brbt_array& temp_array = context->GetConnections_array;
			std::vector<std::string>& temp_alloc = context->GetConnections_alloc;
			const std::vector<Biribit::Connection>& client_result = future.get();

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

			callback(temp_array.convert<brbt_Connection_array>());
		});
	});
}

void brbt_GetRemoteClients(brbt_Client client, brbt_id_t id_conn, brbt_RemoteClient_callback callback)
{
	brbt_context* context = (brbt_context*)client; Biribit::Client* cl = &(context->client);
	auto future = context->client.GetRemoteClients(id_conn).share();
	context->tasks.push_back([context, future, callback]() -> bool
	{
		return Future_Then(future, [context, future, callback]
		{
			callback(alloc_RemoteClient_array(context, future.get()));
		});
	});
}

void brbt_GetRooms(brbt_Client client, brbt_id_t id_conn, brbt_Room_callback callback)
{
	brbt_context* context = (brbt_context*)client; Biribit::Client* cl = &(context->client);
	auto future = cl->GetRooms(id_conn).share();
	context->tasks.push_back([context, future, callback]() -> bool
	{
		return Future_Then(future, [context, future, callback]
		{
			callback(alloc_Room_array(context, future.get()));
		});
	});
}

void brbt_UpdateCallbacks(brbt_Client client)
{
	brbt_context* context = (brbt_context*)client; Biribit::Client* cl = &(context->client);
	for (auto it = context->tasks.begin(); it != context->tasks.end();)
		if ((*it)())
			it = context->tasks.erase(it);
		else
			it++;
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

void brbt_CreateRoom(brbt_Client client, brbt_id_t id_conn, brbt_slot_id_t num_slots)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->CreateRoom(id_conn, num_slots);
}

void brbt_CreateRoomAndJoinSlot(brbt_Client client, brbt_id_t id_conn, brbt_slot_id_t num_slots, brbt_slot_id_t slot_to_join_id)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->CreateRoom(id_conn, num_slots, slot_to_join_id);
}

void brbt_JoinRandomOrCreateRoom(brbt_Client client, brbt_id_t id_conn, brbt_slot_id_t num_slots)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->JoinRandomOrCreateRoom(id_conn, num_slots);
}

void brbt_JoinRoom(brbt_Client client, brbt_id_t id_conn, brbt_id_t room_id)
{
	brbt_context* context = (brbt_context*) client; Biribit::Client* cl = &(context->client);
	cl->JoinRoom(id_conn, room_id);
}

void brbt_JoinRoomAndSlot(brbt_Client client, brbt_id_t id_conn, brbt_id_t room_id, brbt_slot_id_t slot_id)
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

template<class T, class U> std::unique_ptr<T> unique_ptr_cast(std::unique_ptr<U>& ptr)
{
	return std::unique_ptr<T>(static_cast<T*>(ptr.release()));
}

void brbt_HandleEvent(brbt_Client client, const brbt_EventCallbackTable* table, std::unique_ptr<Biribit::ErrorEvent> evnt)
{
	brbt_ErrorEvent ret;
	ret.which = (brbt_ErrorId) evnt->which;
	table->error(&ret);
}

void brbt_HandleEvent(brbt_Client client, const brbt_EventCallbackTable* table, std::unique_ptr<Biribit::ServerListEvent> evnt)
{
	brbt_ServerListEvent ret;
	table->server_list(&ret);
}

void brbt_HandleEvent(brbt_Client client, const brbt_EventCallbackTable* table, std::unique_ptr<Biribit::ConnectionEvent> evnt)
{
	brbt_ConnectionEvent ret;
	ret.type = (brbt_ConnectionEventType) evnt->type;
	ret.connection.id = evnt->connection.id;
	ret.connection.name = evnt->connection.name.c_str();
	ret.connection.ping = evnt->connection.ping;

	table->connection(&ret);
}

void brbt_HandleEvent(brbt_Client client, const brbt_EventCallbackTable* table, std::unique_ptr<Biribit::ServerStatusEvent> evnt)
{
	brbt_ServerStatusEvent ret;
	ret.connection = evnt->connection;
	ret.clients = alloc_RemoteClient_array((brbt_context*)client, evnt->clients);

	table->server_status(&ret);
}

void brbt_HandleEvent(brbt_Client client, const brbt_EventCallbackTable* table, std::unique_ptr<Biribit::RemoteClientEvent> evnt)
{
	brbt_RemoteClientEvent ret;
	ret.type = (brbt_RemoteClientEventType) evnt->type;
	ret.client.id = evnt->client.id;
	ret.client.name = evnt->client.name.c_str();
	ret.client.appid = evnt->client.name.c_str();
	ret.self = evnt->self ? BRBT_TRUE : BRBT_FALSE;

	table->remote_client(&ret);
}

void brbt_HandleEvent(brbt_Client client, const brbt_EventCallbackTable* table, std::unique_ptr<Biribit::RoomListEvent> evnt)
{
	brbt_RoomListEvent ret;
	ret.connection = evnt->connection;
	ret.rooms = alloc_Room_array((brbt_context*)client, evnt->rooms);
	
	table->room_list(&ret);
}

void brbt_HandleEvent(brbt_Client client, const brbt_EventCallbackTable* table, std::unique_ptr<Biribit::JoinedRoomEvent> evnt)
{
	brbt_JoinedRoomEvent ret;
	ret.connection = evnt->connection;
	ret.room_id = evnt->room_id;
	ret.slot_id = evnt->slot_id;

	table->joined_room(&ret);
}

void brbt_HandleEvent(brbt_Client client, const brbt_EventCallbackTable* table, std::unique_ptr<Biribit::BroadcastEvent> evnt)
{
	brbt_BroadcastEvent ret;
	ret.connection = evnt->connection;
	ret.when = evnt->when;
	ret.room_id = evnt->room_id;
	ret.slot_id = evnt->slot_id;
	ret.data_size = evnt->data.getDataSize();
	ret.data = evnt->data.getData();

	table->broadcast(&ret);
}

void brbt_HandleEvent(brbt_Client client, const brbt_EventCallbackTable* table, std::unique_ptr<Biribit::EntriesEvent> evnt)
{
	brbt_EntriesEvent res;
	res.connection = evnt->connection;
	res.room_id = evnt->room_id;
	
	table->entries(&res);
}

void brbt_PullEvent(brbt_Client client, const brbt_EventCallbackTable* table)
{
	std::unique_ptr<Biribit::Event> evnt;
	brbt_context* context = (brbt_context*)client; Biribit::Client* cl = &(context->client);
	while ((evnt = cl->PullEvent()) != nullptr)
	{
		switch (evnt->id)
		{
		case Biribit::EVENT_NONE_ID:
			break;
		case Biribit::ErrorEvent::EVENT_ID:
			brbt_HandleEvent(client, table, unique_ptr_cast<Biribit::ErrorEvent>(evnt));
			break;
		case Biribit::ServerListEvent::EVENT_ID:
			brbt_HandleEvent(client, table, unique_ptr_cast<Biribit::ServerListEvent>(evnt));
			break;
		case Biribit::ConnectionEvent::EVENT_ID:
			brbt_HandleEvent(client, table, unique_ptr_cast<Biribit::ConnectionEvent>(evnt));
			break;
		case Biribit::ServerStatusEvent::EVENT_ID:
			brbt_HandleEvent(client, table, unique_ptr_cast<Biribit::ServerStatusEvent>(evnt));
			break;
		case Biribit::RemoteClientEvent::EVENT_ID:
			brbt_HandleEvent(client, table, unique_ptr_cast<Biribit::RemoteClientEvent>(evnt));
			break;
		case Biribit::RoomListEvent::EVENT_ID:
			brbt_HandleEvent(client, table, unique_ptr_cast<Biribit::RoomListEvent>(evnt));
			break;
		case Biribit::JoinedRoomEvent::EVENT_ID:
			brbt_HandleEvent(client, table, unique_ptr_cast<Biribit::JoinedRoomEvent>(evnt));
			break;
		case Biribit::BroadcastEvent::EVENT_ID:
			brbt_HandleEvent(client, table, unique_ptr_cast<Biribit::BroadcastEvent>(evnt));
			break;
		case Biribit::EntriesEvent::EVENT_ID:
			brbt_HandleEvent(client, table, unique_ptr_cast<Biribit::EntriesEvent>(evnt));
			break;
		}
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
