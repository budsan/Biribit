#include <Biribit/Client/BiribitClient.h>

#include <Biribit/Packet.h>
#include <Biribit/Common/PrintLog.h>
#include <Biribit/Common/BiribitMessageIdentifiers.h>
#include <Biribit/Common/Debug.h>
#include <Biribit/Common/RefSwap.h>
#include <Biribit/Common/TaskPool.h>
#include <Biribit/Common/Types.h>
#include <Biribit/Common/Generic.h>

#include <map>
#include <queue>
#include <vector>
#include <array>
#include <functional>

//RakNet
#include <MessageIdentifiers.h>
#include <RakPeerInterface.h>
#include <RakNetStatistics.h>
#include <RakNetTypes.h>
#include <BitStream.h>
#include <PacketLogger.h>
#include <RakNetTypes.h>
#include <StringCompressor.h>
#include <GetTime.h>

namespace Biribit
{

ServerInfo::ServerInfo()
	: name()
	, addr(RakNet::UNASSIGNED_SYSTEM_ADDRESS.ToString(false))
	, ping(0)
	, port(0)
	, passwordProtected(false)
{
}

ServerConnection::ServerConnection() : id(ServerConnection::UNASSIGNED_ID)
{
}

RemoteClient::RemoteClient() : id(RemoteClient::UNASSIGNED_ID)
{
}

ClientParameters::ClientParameters()
{
}

ClientParameters::ClientParameters(const std::string& name, const std::string& appid)
	: name(name)
	, appid(appid)
{
}

Room::Room()
	: id(Room::UNASSIGNED_ID)
{
}

Received::Received()
	: when(0)
	, connection(ServerConnection::UNASSIGNED_ID)
	, room_id(Room::UNASSIGNED_ID)
	, slot_id(0)
	, data()
{
}

Received::Received(Received&& to_move)
{
	std::swap(when, to_move.when);
	std::swap(connection, to_move.connection);
	std::swap(room_id, to_move.room_id);
	std::swap(slot_id, to_move.slot_id);
	data = std::move(to_move.data);
}

Entry::Entry()
	: id(UNASSIGNED_ID)
	, from_slot(0)
	, data()
{
}

Entry::Entry(Entry&& to_move)
{
	this->operator=(std::move(to_move));
}

Entry::~Entry()
{
}

Entry& Entry::operator=(Entry&& to_move)
{
	std::swap(id, to_move.id);
	std::swap(from_slot, to_move.from_slot);
	data = std::move(data);
	return *this;
}


//---------------------------------------------------------------------------//

struct ServerInfoPriv
{
	ServerInfo data;
	ServerConnection::id_t id;
	bool valid;

	ServerInfoPriv();
};

ServerInfoPriv::ServerInfoPriv()
	: id(ServerConnection::UNASSIGNED_ID)
	, valid(false)
{
}

//---------------------------------------------------------------------------//

class ServerConnectionPriv
{
public:
	ServerConnection data;
	RakNet::SystemAddress addr;

	ClientParameters requested;
	RemoteClient::id_t selfId;

	std::vector<RemoteClient> clients;
	RefSwap<std::vector<RemoteClient>> clientsListReq;

	std::vector<Room> rooms;
	RefSwap<std::vector<Room>> roomsListReq;

	Room::id_t joinedRoom;
	std::uint32_t joinedSlot;

	std::vector<RefSwap<Entry>> joinedRoomEntries;
	std::mutex entriesMutex;

	static Entry EntryDummy;

	ServerConnectionPriv();

	bool isNull();
	unique<Proto::RoomEntriesRequest> UpdateEntries(Proto::RoomEntriesStatus* proto_entries);
	Entry::id_t GetEntriesCount();
	const Entry& GetEntry(Entry::id_t id);
	void ResetEntries();

	void UpdateRemoteClients();
	void UpdateRooms();
};

Entry ServerConnectionPriv::EntryDummy;

ServerConnectionPriv::ServerConnectionPriv()
	: addr(RakNet::UNASSIGNED_SYSTEM_ADDRESS)
	, selfId(RemoteClient::UNASSIGNED_ID)
	, joinedRoom(Room::UNASSIGNED_ID)
	, joinedSlot(0)
	, joinedRoomEntries(1)
{
}

bool ServerConnectionPriv::isNull()
{
	return addr == RakNet::UNASSIGNED_SYSTEM_ADDRESS;
}

unique<Proto::RoomEntriesRequest> ServerConnectionPriv::UpdateEntries(Proto::RoomEntriesStatus* proto_entries)
{
	unique<Proto::RoomEntriesRequest> proto_entriesReq;
	if (proto_entries->has_room_id() && joinedRoom == proto_entries->room_id())
	{
		if (proto_entries->has_journal_size())
		{
			//resize journal
			std::uint32_t journal_size = proto_entries->journal_size();
			{
				std::lock_guard<std::mutex> lock(entriesMutex);
				joinedRoomEntries.resize(journal_size + 1);
			}
			
			//saving entries in journal
			int size = proto_entries->entries_size();
			for (int i = 0; i < size; i++)
			{
				const Proto::RoomEntry& proto_entry = proto_entries->entries(i);
				if (proto_entry.has_id() && proto_entry.has_from_slot() && proto_entry.has_entry_data())
				{
					std::uint32_t id = proto_entry.id();
					if (id > Entry::UNASSIGNED_ID && id <= journal_size)
					{
						RefSwap<Entry>& safe_entry = joinedRoomEntries[proto_entry.id()];
						Entry& entry = safe_entry.back();
						entry.id = proto_entry.id();
						entry.from_slot = proto_entry.from_slot();
						const std::string& data = proto_entry.entry_data();
						entry.data.append(data.c_str(), data.size());
						safe_entry.swap();
					}
				}
			}

			// finding out if there's more entries left to ask for
			for (std::uint32_t i = 1; i < journal_size; i++)
			{
				if (!joinedRoomEntries[i].hasEverSwapped())
				{
					if (proto_entriesReq == nullptr)
						proto_entriesReq = unique<Proto::RoomEntriesRequest>(new Proto::RoomEntriesRequest);
					proto_entriesReq->add_entries_id(i);
				}
			}
		}
	}

	return proto_entriesReq;
}

Entry::id_t ServerConnectionPriv::GetEntriesCount()
{
	if (joinedRoom > Room::UNASSIGNED_ID)
	{
		std::lock_guard<std::mutex> lock(entriesMutex);
		return  joinedRoomEntries.size() - 1;
	}

	return Entry::UNASSIGNED_ID;
}

const Entry& ServerConnectionPriv::GetEntry(Entry::id_t id)
{
	{
		std::lock_guard<std::mutex> lock(entriesMutex);
		if (id > Entry::UNASSIGNED_ID && id < joinedRoomEntries.size())
			return joinedRoomEntries[id].front(nullptr);
	}

	return EntryDummy;
}

void ServerConnectionPriv::ResetEntries()
{
	std::lock_guard<std::mutex> lock(entriesMutex);
	joinedRoomEntries.resize(1);
}

void ServerConnectionPriv::UpdateRemoteClients()
{
	std::vector<RemoteClient>& back = clientsListReq.back();
	back.clear();

	for (auto it = clients.begin(); it != clients.end(); it++)
		if (it->id != RemoteClient::UNASSIGNED_ID)
			back.push_back(*it);

	clientsListReq.swap();
}

void ServerConnectionPriv::UpdateRooms()
{
	std::vector<Room>& back = roomsListReq.back();
	back.clear();

	for (auto it = rooms.begin(); it != rooms.end(); it++)
		if (it->id != Room::UNASSIGNED_ID)
			back.push_back(*it);

	roomsListReq.swap();
}

//---------------------------------------------------------------------------//

const char* StartupResultStr[] = {
	"RAKNET_STARTED",
	"RAKNET_ALREADY_STARTED",
	"INVALID_SOCKET_DESCRIPTORS",
	"INVALID_MAX_CONNECTIONS",
	"SOCKET_FAMILY_NOT_SUPPORTED",
	"SOCKET_PORT_ALREADY_IN_USE",
	"SOCKET_FAILED_TO_BIND",
	"SOCKET_FAILED_TEST_SEND",
	"PORT_CANNOT_BE_ZERO",
	"FAILED_TO_CREATE_NETWORK_THREAD",
	"COULD_NOT_GENERATE_GUID",
	"STARTUP_OTHER_FAILURE"
};

const char* ConnectionAttemptResultStr[] = {
	"CONNECTION_ATTEMPT_STARTED",
	"INVALID_PARAMETER",
	"CANNOT_RESOLVE_DOMAIN_NAME",
	"ALREADY_CONNECTED_TO_ENDPOINT",
	"CONNECTION_ATTEMPT_ALREADY_IN_PROGRESS",
	"SECURITY_INITIALIZATION_FAILED"
};

class ClientImpl
{
public:
	ClientImpl();
	~ClientImpl();

	void Connect(const char* addr = nullptr, unsigned short port = 0, const char* password = nullptr);
	void Disconnect(ServerConnection::id_t id);
	void Disconnect();

	void DiscoverOnLan(unsigned short port);
	void ClearDiscoverInfo();
	void RefreshDiscoverInfo();
	const std::vector<ServerInfo>& GetDiscoverInfo(std::uint32_t* revision);
	const std::vector<ServerConnection>& GetConnections(std::uint32_t* revision);
	const std::vector<RemoteClient>& GetRemoteClients(ServerConnection::id_t id, std::uint32_t* revision);

	RemoteClient::id_t GetLocalClientId(ServerConnection::id_t id);
	void SetLocalClientParameters(ServerConnection::id_t id, const ClientParameters& parameters);

	void RefreshRooms(ServerConnection::id_t id);
	const std::vector<Room>& GetRooms(ServerConnection::id_t id, std::uint32_t* revision);

	void CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots);
	void CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots, std::uint32_t slot_to_join_id);

	void JoinRandomOrCreateRoom(ServerConnection::id_t id, std::uint32_t num_slots);

	void JoinRoom(ServerConnection::id_t id, Room::id_t room_id);
	void JoinRoom(ServerConnection::id_t id, Room::id_t room_id, std::uint32_t slot_id);

	Room::id_t GetJoinedRoomId(ServerConnection::id_t id);
	std::uint32_t GetJoinedRoomSlot(ServerConnection::id_t id);

	void SendBroadcast(ServerConnection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask);
	void SendBroadcast(ServerConnection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask);

	void SendEntry(ServerConnection::id_t id, const Packet& packet);
	void SendEntry(ServerConnection::id_t id, const char* data, unsigned int lenght);

	std::size_t GetDataSizeOfNextReceived();
	std::unique_ptr<Received> PullReceived();

	Entry::id_t GetEntriesCount(ServerConnection::id_t id);
	const Entry& GetEntry(ServerConnection::id_t id, Entry::id_t entryId);

private: 
	RakNet::RakPeerInterface *m_peer;
	unique<TaskPool> m_pool;

	void SendBroadcast(ServerConnection::id_t id, shared<Packet> packet, Packet::ReliabilityBitmask mask);
	void SendEntry(ServerConnection::id_t id, shared<Packet> packet);

	void SendProtocolMessageID(RakNet::MessageID msg, const RakNet::AddressOrGUID systemIdentifier);
	bool WriteMessage(RakNet::BitStream& bstream, RakNet::MessageID msgId, const ::google::protobuf::MessageLite& msg);
	template<typename T> bool ReadMessage(T& msg, RakNet::BitStream& bstream);
	template<typename T> bool ReadMessage(T& msg, Packet& packet);

	static void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
	void RakNetUpdated();
	void HandlePacket(RakNet::Packet*);

	void ConnectedAt(RakNet::SystemAddress);
	void DisconnectFrom(RakNet::SystemAddress);

	enum TypeUpdateRemoteClient {UPDATE_REMOTE_CLIENT, UPDATE_SELF_CLIENT, UPDATE_REMOTE_DISCONNECTION};
	void UpdateRemoteClient(RakNet::SystemAddress addr, const Proto::Client* proto_client, TypeUpdateRemoteClient type);
	void UpdateRoom(RakNet::SystemAddress addr, const Proto::Room* proto_room);

	static void PopulateServerInfo(ServerInfoPriv&, const Proto::ServerInfo*);
	static void PopulateRemoteClient(RemoteClient&, const Proto::Client*);
	static void PopulateRoom(Room&, const Proto::Room*);

	void UpdateDiscoverInfo();
	void UpdateConnections();

	Generic::TempBuffer m_buffer;
	
	std::map<RakNet::SystemAddress, ServerInfoPriv> serverList;
	RefSwap<std::vector<ServerInfo>> serverListReq;

	std::array<ServerConnectionPriv, CLIENT_MAX_CONNECTIONS+1> m_connections;
	RefSwap<std::vector<ServerConnection>> connectionsListReq;
	RakNet::TimeMS m_current;
	RakNet::TimeMS m_lastDirtyDueTime;

	std::queue<std::unique_ptr<Received>> m_receivedPending;
	std::mutex m_receivedMutex;
};

ClientImpl::ClientImpl()
	: m_peer(nullptr)
{
	m_pool = unique<TaskPool>(new TaskPool(1, "Client"));

	m_peer = RakNet::RakPeerInterface::GetInstance();
	m_peer->SetUserUpdateThread(RaknetThreadUpdate, this);
	m_peer->AllowConnectionResponseIPMigration(false);
	m_peer->SetOccasionalPing(true);

	RakNet::SocketDescriptor socketDescriptor;
	socketDescriptor.socketFamily = AF_INET;

	RakNet::StartupResult result = m_peer->Startup(CLIENT_MAX_CONNECTIONS, &socketDescriptor, 1);
	if (result != RakNet::RAKNET_STARTED)
	{
		printLog("Startup failed: %s", StartupResultStr[result]);
		RakNet::RakPeerInterface::DestroyInstance(m_peer);
		m_peer = nullptr;
	}
}

ClientImpl::~ClientImpl()
{
	if (m_peer != nullptr)
	{
		if (m_peer->IsActive()) {
			printLog("Closing client connection...");
			m_peer->Shutdown(60000);
		}

		printLog("Waiting for thread ends...");
		m_pool.reset(nullptr);

		RakNet::RakPeerInterface::DestroyInstance(m_peer);
		m_peer = nullptr;
	}
}

void ClientImpl::RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data)
{
	if (data != nullptr && static_cast<ClientImpl*>(data)->m_peer == peer) {
		static_cast<ClientImpl*>(data)->RakNetUpdated();
	}
}

void ClientImpl::RakNetUpdated()
{
	m_pool->enqueue([this]() {
		RakNet::Packet* p = nullptr;
		while (m_peer != nullptr && (p = m_peer->Receive()) != nullptr) {
			HandlePacket(p);
			m_peer->DeallocatePacket(p);
		}

		m_current = RakNet::GetTimeMS();
		if ((m_current - m_lastDirtyDueTime) > 1000)
			UpdateConnections();
	});
}

void ClientImpl::Connect(const char* addr, unsigned short port, const char* password)
{
	if (port == 0)
		port = SERVER_DEFAULT_PORT;

	static const char localhost[] = "localhost";
	if (addr == nullptr || strcmp(addr, "") == 0)
		addr = localhost;

	m_pool->enqueue([this, addr, port, password]()
	{
		const char* pass = (password == nullptr || (strcmp(password, "") == 0)) ? nullptr : password;
		RakNet::ConnectionAttemptResult car = m_peer->Connect(addr, port, pass, pass != nullptr ? (int)strlen(password) : 0);
		if (car != RakNet::CONNECTION_ATTEMPT_STARTED)
			printLog("Connect failed: %s", ConnectionAttemptResultStr[car]);
	});
}

void ClientImpl::Disconnect(ServerConnection::id_t id)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id]()
	{
		if (id < m_connections.size() && !m_connections[id].isNull())
			m_peer->CloseConnection(m_connections[id].addr, true);
	});
}

void ClientImpl::Disconnect()
{
	m_pool->enqueue([this]()
	{
		for (std::size_t i = 1; i < m_connections.size(); i++)
		{
			if (!m_connections[i].isNull())
				m_peer->CloseConnection(m_connections[i].addr, true);
		}
	});
}

void ClientImpl::DiscoverOnLan(unsigned short port)
{
	if (port == 0)
		port = SERVER_DEFAULT_PORT;

	m_pool->enqueue([this, port]() {
		printLog("Discovering on LAN in port %d...", port);
		m_peer->Ping("255.255.255.255", port, false);
	});
}

void ClientImpl::RefreshDiscoverInfo()
{
	m_pool->enqueue([this]()
	{
		for (auto it = serverList.begin(); it != serverList.end(); it++)
		{
			ServerInfoPriv& info = it->second;
			m_peer->Ping(it->first.ToString(false), it->first.GetPort(), false);
			it->second.valid = false;
		}
	});
}

void ClientImpl::ClearDiscoverInfo()
{
	m_pool->enqueue([this]()
	{
		bool modified = false;
		auto it = serverList.begin();
		while (it != serverList.end())
		{
			if (it->second.id == ServerConnection::UNASSIGNED_ID)
			{
				it = serverList.erase(it);
				modified = true;
			}
			else
				it++;
		}

		if (modified)
			UpdateDiscoverInfo();
	});
}

const std::vector<ServerInfo>& ClientImpl::GetDiscoverInfo(std::uint32_t* revision)
{
	return serverListReq.front(revision);
}

const std::vector<ServerConnection>& ClientImpl::GetConnections(std::uint32_t* revision)
{
	return connectionsListReq.front(revision);
}

const std::vector<RemoteClient>& ClientImpl::GetRemoteClients(ServerConnection::id_t id, std::uint32_t* revision)
{
	static std::vector<RemoteClient> empty;
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return empty;

	return m_connections[id].clientsListReq.front(revision);
}

RemoteClient::id_t ClientImpl::GetLocalClientId(ServerConnection::id_t id)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return RemoteClient::UNASSIGNED_ID;

	return m_connections[id].selfId;
}

void ClientImpl::SetLocalClientParameters(ServerConnection::id_t id, const ClientParameters& _parameters)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	ClientParameters parameters = _parameters;
	m_pool->enqueue([this, id, parameters]()
	{
		ServerConnectionPriv& conn = m_connections[id];
		if (conn.isNull())
			return;

		bool ready = false;
		Proto::ClientUpdate proto_update;
		proto_update.set_name(parameters.name);
		if (!parameters.appid.empty())
			proto_update.set_appid(parameters.appid);
		conn.requested = parameters;

		RakNet::BitStream bstream;
		WriteMessage(bstream, ID_CLIENT_UPDATE_STATUS, proto_update);
		m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, conn.addr, false);
	});
}

void ClientImpl::RefreshRooms(ServerConnection::id_t id)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id]()
	{
		ServerConnectionPriv& conn = m_connections[id];
		SendProtocolMessageID(ID_ROOM_LIST_REQUEST, conn.addr);
	});
}

const std::vector<Room>& ClientImpl::GetRooms(ServerConnection::id_t id, std::uint32_t* revision)
{
	static std::vector<Room> empty;
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return empty;

	return m_connections[id].roomsListReq.front(revision);
}

void ClientImpl::CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id, num_slots]()
	{
		ServerConnectionPriv& conn = m_connections[id];
		if (conn.isNull())
			return;

		Proto::RoomCreate proto_create;
		proto_create.set_client_slots(num_slots);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_ROOM_CREATE_REQUEST, proto_create))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, conn.addr, false);
	});
}

void ClientImpl::CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots, std::uint32_t slot_to_join_id)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id, num_slots, slot_to_join_id]()
	{
		ServerConnectionPriv& conn = m_connections[id];
		if (conn.isNull())
			return;

		Proto::RoomCreate proto_create;
		proto_create.set_client_slots(num_slots);
		proto_create.set_slot_to_join(slot_to_join_id);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_ROOM_CREATE_REQUEST, proto_create))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, conn.addr, false);
	});
}

void ClientImpl::JoinRandomOrCreateRoom(ServerConnection::id_t id, std::uint32_t num_slots)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id, num_slots]()
	{
		ServerConnectionPriv& conn = m_connections[id];
		if (conn.isNull())
			return;

		Proto::RoomCreate proto_create;
		proto_create.set_client_slots(num_slots);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_ROOM_JOIN_RANDOM_OR_CREATE_REQUEST, proto_create))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, conn.addr, false);
	});
}

void ClientImpl::JoinRoom(ServerConnection::id_t id, Room::id_t room_id)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id, room_id]()
	{
		ServerConnectionPriv& conn = m_connections[id];
		if (conn.isNull())
			return;

		Proto::RoomJoin proto_join;
		proto_join.set_id(room_id);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_ROOM_JOIN_REQUEST, proto_join))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, conn.addr, false);
	});
}

void ClientImpl::JoinRoom(ServerConnection::id_t id, Room::id_t room_id, std::uint32_t slot_id)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id, room_id, slot_id]()
	{
		ServerConnectionPriv& conn = m_connections[id];
		if (conn.isNull())
			return;

		Proto::RoomJoin proto_join;
		proto_join.set_id(room_id);
		proto_join.set_slot_to_join(slot_id);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_ROOM_JOIN_REQUEST, proto_join))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, conn.addr, false);
	});
}

Room::id_t ClientImpl::GetJoinedRoomId(ServerConnection::id_t id)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return RemoteClient::UNASSIGNED_ID;

	return m_connections[id].joinedRoom;
}

std::uint32_t ClientImpl::GetJoinedRoomSlot(ServerConnection::id_t id)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return RemoteClient::UNASSIGNED_ID;

	return m_connections[id].joinedSlot;
}

void ClientImpl::SendBroadcast(ServerConnection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	shared<Packet> shared_packet(new Packet());
	shared_packet->append(packet.getData(), packet.getDataSize());
	SendBroadcast(id, shared_packet, mask);
}

void ClientImpl::SendBroadcast(ServerConnection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	shared<Packet> shared_packet(new Packet());
	shared_packet->append(data, lenght);
	SendBroadcast(id, shared_packet, mask);
}

void ClientImpl::SendBroadcast(ServerConnection::id_t id, shared<Packet> packet, Packet::ReliabilityBitmask mask = Packet::Unreliable)
{
	shared<Packet> shared_packet = packet;
	PacketReliability reliability;
	switch (mask)
	{
	case Packet::Unreliable:
		reliability = UNRELIABLE;
		break;
	case Packet::Reliable:
		reliability = RELIABLE;
		break;
	case Packet::Ordered:
	case Packet::ReliableOrdered:
		reliability = RELIABLE_ORDERED;
		break;
	default:
		reliability = UNRELIABLE;
		break;
	}

	m_pool->enqueue([this, id, shared_packet, reliability]()
	{
		ServerConnectionPriv& conn = m_connections[id];
		if (conn.isNull())
			return;

		RakNet::BitStream bstream;
		bstream.Write((RakNet::MessageID) ID_TIMESTAMP);
		bstream.Write(RakNet::GetTime());
		bstream.Write((RakNet::MessageID) ID_SEND_BROADCAST_TO_ROOM);
		bstream.Write((std::uint8_t) reliability);

		const char* data[2] = { (const char*)bstream.GetData(), (const char*)shared_packet->getData() };
		int lengths[2] = { (int)bstream.GetNumberOfBytesUsed(), (int)shared_packet->getDataSize() };
		m_peer->SendList(data, lengths, 2, HIGH_PRIORITY, reliability, id & 0xFF, conn.addr, false);
	});
}

void ClientImpl::SendEntry(ServerConnection::id_t id, const Packet& packet)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	shared<Packet> shared_packet(new Packet());
	shared_packet->append(packet.getData(), packet.getDataSize());
	SendEntry(id, shared_packet);
}

void ClientImpl::SendEntry(ServerConnection::id_t id, const char* data, unsigned int lenght)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	shared<Packet> shared_packet(new Packet());
	shared_packet->append(data, lenght);
	SendEntry(id, shared_packet);
}

void ClientImpl::SendEntry(ServerConnection::id_t id, shared<Packet> packet)
{
	shared<Packet> shared_packet = packet;
	m_pool->enqueue([this, id, shared_packet]()
	{
		ServerConnectionPriv& conn = m_connections[id];
		if (conn.isNull())
			return;

		RakNet::BitStream bstream;
		bstream.Write((RakNet::MessageID) ID_TIMESTAMP);
		bstream.Write(RakNet::GetTime());
		bstream.Write((RakNet::MessageID) ID_SEND_ENTRY_TO_ROOM);

		const char* data[2] = { (const char*)bstream.GetData(), (const char*)shared_packet->getData() };
		int lengths[2] = { (int)bstream.GetNumberOfBytesUsed(), (int)shared_packet->getDataSize() };
		m_peer->SendList(data, lengths, 2, MEDIUM_PRIORITY, RELIABLE, id & 0xFF, conn.addr, false);
	});
}

std::size_t ClientImpl::GetDataSizeOfNextReceived()
{
	std::lock_guard<std::mutex> lock(m_receivedMutex);
	if (m_receivedPending.empty())
		return 0;

	return m_receivedPending.front()->data.getDataSize();
}

std::unique_ptr<Received> ClientImpl::PullReceived()
{
	std::lock_guard<std::mutex> lock(m_receivedMutex);
	if (m_receivedPending.empty())
		return nullptr;

	std::unique_ptr<Received> ptr = std::move(m_receivedPending.front());
	m_receivedPending.pop();
	return std::move(ptr);
}

Entry::id_t ClientImpl::GetEntriesCount(ServerConnection::id_t id)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return 0;

	return m_connections[id].GetEntriesCount();
}

const Entry& ClientImpl::GetEntry(ServerConnection::id_t id, Entry::id_t entryId)
{
	if (id == ServerConnection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return ServerConnectionPriv::EntryDummy;

	return m_connections[id].GetEntry(entryId);
}

void ClientImpl::SendProtocolMessageID(RakNet::MessageID msg, const RakNet::AddressOrGUID systemIdentifier)
{
	Packet tosend; tosend << msg;
	m_peer->Send((const char*)tosend.getData(), (int)tosend.getDataSize(), LOW_PRIORITY, RELIABLE, 0, systemIdentifier, false);
}

bool ClientImpl::WriteMessage(RakNet::BitStream& bstream, RakNet::MessageID msgId, const ::google::protobuf::MessageLite& msg)
{
	std::size_t size = (std::size_t) msg.ByteSize();
	m_buffer.Ensure(size);
	if (msg.SerializeToArray(m_buffer.data, m_buffer.size))
	{
		bstream.Write((RakNet::MessageID) msgId);
		bstream.Write(m_buffer.data, size);
		return true;
	}

	return false;
}

template<typename T> bool ClientImpl::ReadMessage(T& msg, Packet& packet)
{
	std::size_t size = packet.getDataSize() - packet.getReadPos();
	m_buffer.Ensure(size);
	packet.read(m_buffer.data, size);
	return msg.ParseFromArray(m_buffer.data, (int) size);
}

template<typename T> bool ClientImpl::ReadMessage(T& msg, RakNet::BitStream& bstream)
{
	std::size_t size = BITS_TO_BYTES(bstream.GetNumberOfUnreadBits());
	m_buffer.Ensure(size);
	bstream.Read(m_buffer.data, size);
	return msg.ParseFromArray(m_buffer.data, size);
}

void ClientImpl::HandlePacket(RakNet::Packet* pPacket)
{
	RakNet::BitStream stream(pPacket->data, pPacket->length, false);
	RakNet::MessageID packetIdentifier;
	RakNet::Time timeStamp = 0;

	stream.Read(packetIdentifier);
	if (packetIdentifier == ID_TIMESTAMP)
	{
		RakAssert(pPacket->length > (sizeof(RakNet::MessageID) + sizeof(RakNet::Time)));
		stream.Read(timeStamp);
		stream.Read(packetIdentifier);
	}

	// Check if this is a network message packet
	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		DisconnectFrom(pPacket->systemAddress);
		break;
	case ID_ALREADY_CONNECTED:
		printLog("ALREADY CONNECTED");
		break;
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		printLog("ID_INCOMPATIBLE_PROTOCOL_VERSION");
		break;
	case ID_ADVERTISE_SYSTEM:
		stream.Read(packetIdentifier);
		if (ID_SERVER_INFO_RESPONSE)
		{
			Proto::ServerInfo proto_info;
			if (ReadMessage(proto_info, stream))
			{
				ServerInfoPriv& si = serverList[pPacket->systemAddress];
				PopulateServerInfo(si, &proto_info);
				UpdateDiscoverInfo();
			}
		}
		break;
	case ID_UNCONNECTED_PONG:
	{
		RakNet::TimeMS pingTime;
		RakNet::TimeMS current = RakNet::GetTimeMS();
		stream.Read(pingTime);

		ServerInfoPriv& si = serverList[pPacket->systemAddress];
		si.data.ping = current - pingTime;
		UpdateDiscoverInfo();
		
		Packet tosend;
		tosend << (RakNet::MessageID) ID_SERVER_INFO_REQUEST;
		m_peer->AdvertiseSystem(
			pPacket->systemAddress.ToString(false),
			pPacket->systemAddress.GetPort(),
			(const char*) tosend.getData(),
			tosend.getDataSize());

		break;
	}
	case ID_REMOTE_DISCONNECTION_NOTIFICATION:
		printLog("ID_REMOTE_DISCONNECTION_NOTIFICATION");
		DisconnectFrom(pPacket->systemAddress);
		break;
	case ID_REMOTE_CONNECTION_LOST:
		printLog("ID_REMOTE_CONNECTION_LOST");
		DisconnectFrom(pPacket->systemAddress);
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION:
		printLog("ID_REMOTE_NEW_INCOMING_CONNECTION");
		break;
	case ID_CONNECTION_BANNED:
		printLog("Banned from server.");
		DisconnectFrom(pPacket->systemAddress);
		break;
	case ID_CONNECTION_ATTEMPT_FAILED:
		printLog("Connection attempt failed");
		break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		printLog("Server is full.");
		break;
	case ID_INVALID_PASSWORD:
		printLog("ID_INVALID_PASSWORD");
		break;
	case ID_CONNECTION_LOST:
		printLog("Lost connection form server.");
		DisconnectFrom(pPacket->systemAddress);
		break;
	case ID_CONNECTION_REQUEST_ACCEPTED:
		ConnectedAt(pPacket->systemAddress);
		break;
	case ID_ERROR_CODE:
	{
		std::uint32_t errorCode;
		stream.Read(errorCode);
		switch (errorCode)
		{
		case WARN_CLIENT_NAME_IN_USE:
			printLog("Error code WARN_CLIENT_NAME_IN_USE"); break;
		case WARN_CANNOT_LIST_ROOMS_WITHOUT_APPID:
			printLog("Error code WARN_CANNOT_LIST_ROOMS_WITHOUT_APPID"); break;
		case WARN_CANNOT_CREATE_ROOM_WITHOUT_APPID:
			printLog("Error code WARN_CANNOT_CREATE_ROOM_WITHOUT_APPID"); break;
		case WARN_CANNOT_CREATE_ROOM_WITH_WRONG_SLOT_NUMBER:
			printLog("Error code WARN_CANNOT_CREATE_ROOM_WITH_WRONG_SLOT_NUMBER"); break;
		case WARN_CANNOT_CREATE_ROOM_WITH_TOO_MANY_SLOTS:
			printLog("Error code WARN_CANNOT_CREATE_ROOM_WITH_TOO_MANY_SLOTS"); break;
		case WARN_CANNOT_JOIN_WITHOUT_ROOM_ID:
			printLog("Error code WARN_CANNOT_JOIN_WITHOUT_ROOM_ID"); break;
		case WARN_CANNOT_JOIN_TO_UNEXISTING_ROOM:
			printLog("Error code WARN_CANNOT_JOIN_TO_UNEXISTING_ROOM"); break;
		case WARN_CANNOT_JOIN_TO_OTHER_APP_ROOM:
			printLog("Error code WARN_CANNOT_JOIN_TO_OTHER_APP_ROOM"); break;
		case WARN_CANNOT_JOIN_TO_OCCUPIED_SLOT:
			printLog("Error code WARN_CANNOT_JOIN_TO_OCCUPIED_SLOT"); break;
		case WARN_CANNOT_JOIN_TO_INVALID_SLOT:
			printLog("Error code WARN_CANNOT_JOIN_TO_INVALID_SLOT"); break;
		case WARN_CANNOT_JOIN_TO_FULL_ROOM:
			printLog("Error code WARN_CANNOT_JOIN_TO_FULL_ROOM"); break;
		}
		break;
	}
	case ID_SERVER_INFO_REQUEST:
		BIRIBIT_WARN("Nothing to do with ID_SERVER_INFO_REQUEST");
		break;
	case ID_SERVER_INFO_RESPONSE:
	{
		Proto::ServerInfo proto_info;
		if (ReadMessage(proto_info, stream))
		{
			ServerInfoPriv& si = serverList[pPacket->systemAddress];
			PopulateServerInfo(si, &proto_info);
			if (si.id != ServerConnection::UNASSIGNED_ID)
			{
				ServerConnectionPriv& sc = m_connections[si.id];
				sc.data.name = proto_info.name();
				UpdateConnections();
			}

			UpdateDiscoverInfo();
		}

		break;
	}
	case ID_CLIENT_SELF_STATUS:
	{
		Proto::Client proto_client;
		if (ReadMessage(proto_client, stream))
			UpdateRemoteClient(pPacket->systemAddress, &proto_client, UPDATE_SELF_CLIENT);
		break;
	}
	case ID_SERVER_STATUS_REQUEST:
		BIRIBIT_WARN("Nothing to do with ID_SERVER_STATUS_REQUEST");
		break;
	case ID_SERVER_STATUS_RESPONSE:
	{
		Proto::ServerStatus proto_status;
		if (ReadMessage(proto_status, stream))
		{
			ServerInfoPriv& si = serverList[pPacket->systemAddress];
			BIRIBIT_ASSERT(si.id != ServerConnection::UNASSIGNED_ID);
			ServerConnectionPriv& sc = m_connections[si.id];

			std::uint32_t max = 0;
			int clients_size = proto_status.clients_size();
			for (int i = 0; i < clients_size; i++)
				if (proto_status.clients(i).has_id())
					max = std::max(proto_status.clients(i).id(), max);

			sc.clients.resize(max + 1);
			for (int i = 0; i < clients_size; i++) {
				const Proto::Client& proto_client = proto_status.clients(i);
				if (proto_client.has_id())
					PopulateRemoteClient(sc.clients[proto_client.id()], &proto_client);
			}

			sc.UpdateRemoteClients();
		}
		break;
	}
	case ID_CLIENT_UPDATE_STATUS:
		BIRIBIT_WARN("Nothing to do with ID_CLIENT_UPDATE_STATUS");
		break;
	case ID_CLIENT_STATUS_UPDATED:
	{
		Proto::Client proto_client;
		if (ReadMessage(proto_client, stream))
			UpdateRemoteClient(pPacket->systemAddress, &proto_client, UPDATE_REMOTE_CLIENT);
		break;
	}
	case ID_CLIENT_DISCONNECTED:
	{
		Proto::Client proto_client;
		if (ReadMessage(proto_client, stream))
			UpdateRemoteClient(pPacket->systemAddress, &proto_client, UPDATE_REMOTE_DISCONNECTION);
		break;
	}
	case ID_ROOM_LIST_REQUEST:
		BIRIBIT_WARN("Nothing to do with ID_ROOM_LIST_REQUEST");
		break;
	case ID_ROOM_LIST_RESPONSE:
	{
		Proto::RoomList proto_list;
		if (ReadMessage(proto_list, stream))
		{
			ServerInfoPriv& si = serverList[pPacket->systemAddress];
			BIRIBIT_ASSERT(si.id != ServerConnection::UNASSIGNED_ID);
			ServerConnectionPriv& sc = m_connections[si.id];

			std::uint32_t max = 0;
			int rooms_size = proto_list.rooms_size();
			for (int i = 0; i < rooms_size; i++)
				if (proto_list.rooms(i).has_id())
					max = std::max(proto_list.rooms(i).id(), max);

			sc.rooms.resize(max + 1);
			for (int i = 0; i < rooms_size; i++) {
				const Proto::Room& proto_room = proto_list.rooms(i);
				if (proto_room.has_id())
					PopulateRoom(sc.rooms[proto_room.id()], &proto_room);
			}

			sc.UpdateRooms();
		}
		break;
	}
	case ID_ROOM_CREATE_REQUEST:
		BIRIBIT_WARN("Nothing to do with ID_ROOM_CREATE_REQUEST");
		break;
	case ID_ROOM_STATUS:
	{
		Proto::Room proto_room;
		if (ReadMessage(proto_room, stream))
			UpdateRoom(pPacket->systemAddress, &proto_room);
		break;
	}
	case ID_ROOM_JOIN_REQUEST:
		BIRIBIT_WARN("Nothing to do with ID_ROOM_JOIN_REQUEST");
		break;
	case ID_ROOM_JOIN_RESPONSE:
	{
		Proto::RoomJoin proto_join;
		if (ReadMessage(proto_join, stream))
		{
			ServerInfoPriv& si = serverList[pPacket->systemAddress];
			BIRIBIT_ASSERT(si.id != ServerConnection::UNASSIGNED_ID);
			ServerConnectionPriv& sc = m_connections[si.id];
			if (proto_join.has_id())
			{
				if (sc.joinedRoom != proto_join.id())
				{
					sc.joinedRoom = proto_join.id();
					sc.ResetEntries();
				}
				
				if (proto_join.has_slot_to_join())
					sc.joinedSlot = proto_join.slot_to_join();
			}	
		}
		break;
	}
	case ID_SEND_BROADCAST_TO_ROOM:
		BIRIBIT_WARN("Nothing to do with ID_SEND_BROADCAST_TO_ROOM");
		break;
	case ID_BROADCAST_FROM_ROOM:
	{
		ServerInfoPriv& si = serverList[pPacket->systemAddress];
		if (si.id != ServerConnection::UNASSIGNED_ID)
		{
			ServerConnectionPriv& sc = m_connections[si.id];
			std::unique_ptr<Received> recv(new Received);
			recv->connection = si.id;
			recv->room_id = sc.joinedRoom;
			stream.Read(recv->slot_id);

			if (timeStamp != 0)
				recv->when = timeStamp;

			std::size_t size = BITS_TO_BYTES(stream.GetNumberOfUnreadBits());
			m_buffer.Ensure(size);
			stream.Read(m_buffer.data, size);
			recv->data.append(m_buffer.data, size);

			std::lock_guard<std::mutex> lock(m_receivedMutex);
			m_receivedPending.push(std::move(recv));
		}
		break;
	}
	case ID_JOURNAL_ENTRIES_REQUEST:
		BIRIBIT_WARN("Nothing to do with ID_JOURNAL_ENTRIES_REQUEST");
		break;
	case ID_JOURNAL_ENTRIES_STATUS:
	{
		Proto::RoomEntriesStatus proto_entries;
		if (ReadMessage(proto_entries, stream))
		{
			ServerInfoPriv& si = serverList[pPacket->systemAddress];
			if (si.id != ServerConnection::UNASSIGNED_ID)
			{
				ServerConnectionPriv& sc = m_connections[si.id];
				unique<Proto::RoomEntriesRequest> proto_entriesReq = sc.UpdateEntries(&proto_entries);
				if (proto_entriesReq != nullptr)
				{
					Proto::RoomEntriesRequest* proto_entriesReqPtr = proto_entriesReq.release();
					RakNet::BitStream bstream;
					WriteMessage(bstream, ID_JOURNAL_ENTRIES_REQUEST, *proto_entriesReqPtr);
					m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, sc.addr, false);
					proto_entriesReq = unique<Proto::RoomEntriesRequest>(proto_entriesReqPtr);
				}
			}
		}
		break;
	}
	case ID_SEND_ENTRY_TO_ROOM:
		BIRIBIT_WARN("Nothing to do with ID_JOURNAL_ENTRIES_REQUEST");
		break;
	default:
		printLog("UNKNOWN PACKET IDENTIFIER");
		break;
	}
}

void ClientImpl::ConnectedAt(RakNet::SystemAddress addr)
{
	//Looking for room in m_connections vector
	std::size_t i = 1;
	for (; i < m_connections.size(); i++)
	{
		if (m_connections[i].isNull())
		{
			ServerConnectionPriv& sc = m_connections[i];
			sc.addr = addr;
			sc.data.id = i;
			break;
		}
	}

	if (i == m_connections.size())
		return;

	ServerInfoPriv& si = serverList[addr];
	si.id = i;

	SendProtocolMessageID(ID_SERVER_INFO_REQUEST, addr);
	SendProtocolMessageID(ID_SERVER_STATUS_REQUEST, addr);
	UpdateConnections();
}

void ClientImpl::DisconnectFrom(RakNet::SystemAddress addr)
{
	ServerInfoPriv& si = serverList[addr];
	if (si.id != ServerConnection::UNASSIGNED_ID)
	{
		ServerConnectionPriv& sc = m_connections[si.id];
		if (si.valid)
			printLog("Disconnected from %s.", sc.data.name.c_str());

		sc.addr = RakNet::UNASSIGNED_SYSTEM_ADDRESS;
		sc.selfId = RemoteClient::UNASSIGNED_ID;
		si.id = ServerConnection::UNASSIGNED_ID;
		UpdateConnections();

		sc.requested = ClientParameters();

		sc.joinedRoom = Room::UNASSIGNED_ID;
		sc.joinedSlot = 0;
		sc.ResetEntries();

		sc.clients.clear();
		sc.UpdateRemoteClients();

		sc.rooms.clear();
		sc.UpdateRooms();
	}
}

void ClientImpl::UpdateRemoteClient(RakNet::SystemAddress addr, const Proto::Client* proto_client, TypeUpdateRemoteClient type)
{
	ServerInfoPriv& si = serverList[addr];
	BIRIBIT_ASSERT(si.id != ServerConnection::UNASSIGNED_ID);
	ServerConnectionPriv& sc = m_connections[si.id];
	if (proto_client->has_id())
	{
		std::uint32_t id = proto_client->id();
		if (id >= sc.clients.size())
			sc.clients.resize(id + 1);

		switch (type)
		{
		case UPDATE_SELF_CLIENT:
			sc.selfId = id;
		case UPDATE_REMOTE_CLIENT:
			PopulateRemoteClient(sc.clients[id], proto_client);
			break;
		case UPDATE_REMOTE_DISCONNECTION:
			sc.clients[id].id = RemoteClient::UNASSIGNED_ID;
			sc.selfId = sc.selfId == id ? RemoteClient::UNASSIGNED_ID : sc.selfId;
			break;
		}

		sc.UpdateRemoteClients();
	}
}

void ClientImpl::UpdateRoom(RakNet::SystemAddress addr, const Proto::Room* proto_room)
{
	ServerInfoPriv& si = serverList[addr];
	BIRIBIT_ASSERT(si.id != ServerConnection::UNASSIGNED_ID);
	ServerConnectionPriv& sc = m_connections[si.id];
	if (proto_room->has_id())
	{
		std::uint32_t id = proto_room->id();
		if (id >= sc.rooms.size())
			sc.rooms.resize(id + 1);

		PopulateRoom(sc.rooms[id], proto_room);
		sc.UpdateRooms();
	}
}

void ClientImpl::PopulateServerInfo(ServerInfoPriv& si, const Proto::ServerInfo* proto_info)
{
	if (proto_info->has_name()){
		si.data.name = proto_info->name();
		si.valid = true;
	}
		
	if (proto_info->has_password_protected())
		si.data.passwordProtected = proto_info->password_protected();
}

void ClientImpl::PopulateRemoteClient(RemoteClient& remote, const Proto::Client* proto_client)
{
	if (proto_client->has_id())
		remote.id = proto_client->id();

	if (proto_client->has_name())
		remote.name = proto_client->name();

	if (proto_client->has_appid())
		remote.appid = proto_client->appid();
}

void ClientImpl::PopulateRoom(Room& room, const Proto::Room* proto_room)
{
	if (proto_room->has_id())
		room.id = proto_room->id();

	int joined_id_client_size = proto_room->joined_id_client_size();
	room.slots.resize(joined_id_client_size);
	for (std::size_t i = 0; i < room.slots.size(); i++) {
		room.slots[i] = proto_room->joined_id_client(i);
	}
}

void ClientImpl::UpdateDiscoverInfo()
{
	std::vector<ServerInfo>& back = serverListReq.back();
	back.clear();

	for (auto it = serverList.begin(); it != serverList.end(); it++)
	{
		if (it->second.valid)
		{
			back.push_back(it->second.data);
			back.back().addr = it->first.ToString(false);
			back.back().port = it->first.GetPort();
		}
	}

	serverListReq.swap();
}

void ClientImpl::UpdateConnections()
{
	std::vector<ServerConnection>& back = connectionsListReq.back();
	back.clear();

	auto it = m_connections.begin(); it++;
	for (; it != m_connections.end(); it++)
	{
		if (!it->isNull()) {
			it->data.ping = m_peer->GetAveragePing(it->addr);
			back.push_back(it->data);
		}
	}

	connectionsListReq.swap();
	RakNet::TimeMS m_lastDirtyDueTime = RakNet::GetTimeMS();
}

//---------------------------------------------------------------------------//

Client::Client() : m_impl(new ClientImpl())
{
}

Client::~Client()
{
	delete m_impl;
}

void Client::Connect(const char* addr, unsigned short port, const char* password)
{
	m_impl->Connect(addr, port, password);
}

void Client::Disconnect(ServerConnection::id_t id)
{
	m_impl->Disconnect(id);
}

void Client::Disconnect()
{
	m_impl->Disconnect();
}

void Client::DiscoverOnLan(unsigned short port)
{
	m_impl->DiscoverOnLan(port);
}

void Client::ClearDiscoverInfo()
{
	m_impl->ClearDiscoverInfo();
}

void Client::RefreshDiscoverInfo()
{
	m_impl->RefreshDiscoverInfo();
}

const std::vector<ServerInfo>& Client::GetDiscoverInfo(std::uint32_t* revision)
{
	return m_impl->GetDiscoverInfo(revision);
}

const std::vector<ServerConnection>& Client::GetConnections(std::uint32_t* revision)
{
	return m_impl->GetConnections(revision);
}

const std::vector<RemoteClient>& Client::GetRemoteClients(ServerConnection::id_t id, std::uint32_t* revision)
{
	return m_impl->GetRemoteClients(id, revision);
}

RemoteClient::id_t Client::GetLocalClientId(ServerConnection::id_t id)
{
	return m_impl->GetLocalClientId(id);
}

void Client::SetLocalClientParameters(ServerConnection::id_t id, const ClientParameters& parameters)
{
	m_impl->SetLocalClientParameters(id, parameters);
}

void Client::RefreshRooms(ServerConnection::id_t id)
{
	m_impl->RefreshRooms(id);
}

const std::vector<Room>& Client::GetRooms(ServerConnection::id_t id, std::uint32_t* revision)
{
	return m_impl->GetRooms(id, revision);
}

void Client::CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots)
{
	m_impl->CreateRoom(id, num_slots);
}

void Client::CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots, std::uint32_t slot_to_join_id)
{
	m_impl->CreateRoom(id, num_slots, slot_to_join_id);
}

void Client::JoinRandomOrCreateRoom(ServerConnection::id_t id, std::uint32_t num_slots)
{
	m_impl->JoinRandomOrCreateRoom(id, num_slots);
}

void Client::JoinRoom(ServerConnection::id_t id, Room::id_t room_id)
{
	m_impl->JoinRoom(id, room_id);
}

void Client::JoinRoom(ServerConnection::id_t id, Room::id_t room_id, std::uint32_t slot_id)
{
	m_impl->JoinRoom(id, room_id, slot_id);
}

Room::id_t Client::GetJoinedRoomId(ServerConnection::id_t id)
{
	return m_impl->GetJoinedRoomId(id);
}

std::uint32_t Client::GetJoinedRoomSlot(ServerConnection::id_t id)
{
	return m_impl->GetJoinedRoomSlot(id);
}

void Client::SendBroadcast(ServerConnection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask)
{
	m_impl->SendBroadcast(id, packet, mask);
}

void Client::SendBroadcast(ServerConnection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask)
{
	m_impl->SendBroadcast(id, data, lenght, mask);
}

void Client::SendEntry(ServerConnection::id_t id, const Packet& packet)
{
	m_impl->SendEntry(id, packet);
}

void Client::SendEntry(ServerConnection::id_t id, const char* data, unsigned int lenght)
{
	m_impl->SendEntry(id, data, lenght);
}

std::size_t Client::GetDataSizeOfNextReceived()
{
	return m_impl->GetDataSizeOfNextReceived();
}

std::unique_ptr<Received> Client::PullReceived()
{
	return m_impl->PullReceived();
}

Entry::id_t Client::GetEntriesCount(ServerConnection::id_t id)
{
	return m_impl->GetEntriesCount(id);
}

const Entry& Client::GetEntry(ServerConnection::id_t id, Entry::id_t entryId)
{
	return m_impl->GetEntry(id, entryId);
}

}