#include "BiribitClientImpl.h"

namespace Biribit
{

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

ClientImpl::ClientImpl()
	: m_peer(nullptr)
{
	for (ConnectionImpl& c : m_connections)
		c.parent = this;

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

void ClientImpl::Disconnect(Connection::id_t id)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
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

void ClientImpl::DiscoverServersOnLAN(unsigned short port)
{
	if (port == 0)
		port = SERVER_DEFAULT_PORT;

	m_pool->enqueue([this, port]() {
		printLog("Discovering on LAN in port %d...", port);
		m_peer->Ping("255.255.255.255", port, false);
	});
}

void ClientImpl::RefreshServerList()
{
	m_pool->enqueue([this]()
	{
		for (auto it = serverList.begin(); it != serverList.end(); it++)
		{
			ServerInfoImpl& info = it->second;
			m_peer->Ping(it->first.ToString(false), it->first.GetPort(), false);
			it->second.valid = false;
		}
	});
}

void ClientImpl::ClearServerList()
{
	m_pool->enqueue([this]()
	{
		bool modified = false;
		auto it = serverList.begin();
		while (it != serverList.end())
		{
			if (it->second.id == Connection::UNASSIGNED_ID)
			{
				it = serverList.erase(it);
				modified = true;
			}
			else
				it++;
		}

		if (modified)
			PushServerListEvent();
	});
}


std::future<std::vector<ServerInfo>> ClientImpl::GetServerList()
{
	return m_pool->enqueue([this]() -> std::vector<ServerInfo>
	{
		std::vector<ServerInfo> serverList;
		UpdateServerList(serverList);
		return serverList;
	});
}

std::future<std::vector<Connection>> ClientImpl::GetConnections()
{
	return m_pool->enqueue([this]() -> std::vector<Connection>
	{
		std::vector<Connection> connections;
		UpdateConnections(connections);
		return connections;
	});
}

std::future<std::vector<RemoteClient>> ClientImpl::GetRemoteClients(Connection::id_t id)
{
	return m_pool->enqueue([this, id]() -> std::vector<RemoteClient>
	{
		std::vector<RemoteClient> remoteClients;
		if (id != Connection::UNASSIGNED_ID && id <= CLIENT_MAX_CONNECTIONS)
			m_connections[id].UpdateRemoteClients(remoteClients);

		return remoteClients;
	});
}

future_vector<Room> ClientImpl::GetRooms(Connection::id_t id)
{
	return m_pool->enqueue([this, id]() -> std::vector<Room>
	{
		std::vector<Room> rooms;
		if (id != Connection::UNASSIGNED_ID && id <= CLIENT_MAX_CONNECTIONS)
			m_connections[id].UpdateRooms(rooms);

		return rooms;
	});
}

RemoteClient::id_t ClientImpl::GetLocalClientId(Connection::id_t id)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return RemoteClient::UNASSIGNED_ID;

	return m_connections[id].selfId;
}

void ClientImpl::SetLocalClientParameters(Connection::id_t id, const ClientParameters& _parameters)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	ClientParameters parameters = _parameters;
	m_pool->enqueue([this, id, parameters]()
	{
		ConnectionImpl& conn = m_connections[id];
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

void ClientImpl::RefreshRooms(Connection::id_t id)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id]()
	{
		ConnectionImpl& conn = m_connections[id];
		SendProtocolMessageID(ID_ROOM_LIST_REQUEST, conn.addr);
	});
}

void ClientImpl::CreateRoom(Connection::id_t id, Room::slot_id_t num_slots)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id, num_slots]()
	{
		ConnectionImpl& conn = m_connections[id];
		if (conn.isNull())
			return;

		Proto::RoomCreate proto_create;
		proto_create.set_client_slots(num_slots);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_ROOM_CREATE_REQUEST, proto_create))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, conn.addr, false);
	});
}

void ClientImpl::CreateRoom(Connection::id_t id, Room::slot_id_t num_slots, Room::slot_id_t slot_to_join_id)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id, num_slots, slot_to_join_id]()
	{
		ConnectionImpl& conn = m_connections[id];
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

void ClientImpl::JoinRandomOrCreateRoom(Connection::id_t id, Room::slot_id_t num_slots)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id, num_slots]()
	{
		ConnectionImpl& conn = m_connections[id];
		if (conn.isNull())
			return;

		Proto::RoomCreate proto_create;
		proto_create.set_client_slots(num_slots);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_ROOM_JOIN_RANDOM_OR_CREATE_REQUEST, proto_create))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, conn.addr, false);
	});
}

void ClientImpl::JoinRoom(Connection::id_t id, Room::id_t room_id)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id, room_id]()
	{
		ConnectionImpl& conn = m_connections[id];
		if (conn.isNull())
			return;

		Proto::RoomJoin proto_join;
		proto_join.set_id(room_id);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_ROOM_JOIN_REQUEST, proto_join))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, conn.addr, false);
	});
}

void ClientImpl::JoinRoom(Connection::id_t id, Room::id_t room_id, Room::slot_id_t slot_id)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	m_pool->enqueue([this, id, room_id, slot_id]()
	{
		ConnectionImpl& conn = m_connections[id];
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

Room::id_t ClientImpl::GetJoinedRoomId(Connection::id_t id)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return RemoteClient::UNASSIGNED_ID;

	return m_connections[id].joinedRoom;
}

Room::slot_id_t ClientImpl::GetJoinedRoomSlot(Connection::id_t id)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return RemoteClient::UNASSIGNED_ID;

	return m_connections[id].joinedSlot;
}

void ClientImpl::SendBroadcast(Connection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	shared<Packet> shared_packet(new Packet());
	shared_packet->append(packet.getData(), packet.getDataSize());
	SendBroadcast(id, shared_packet, mask);
}

void ClientImpl::SendBroadcast(Connection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	shared<Packet> shared_packet(new Packet());
	shared_packet->append(data, lenght);
	SendBroadcast(id, shared_packet, mask);
}

void ClientImpl::SendBroadcast(Connection::id_t id, shared<Packet> packet, Packet::ReliabilityBitmask mask = Packet::Unreliable)
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
		ConnectionImpl& conn = m_connections[id];
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

void ClientImpl::SendEntry(Connection::id_t id, const Packet& packet)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	shared<Packet> shared_packet(new Packet());
	shared_packet->append(packet.getData(), packet.getDataSize());
	SendEntry(id, shared_packet);
}

void ClientImpl::SendEntry(Connection::id_t id, const char* data, unsigned int lenght)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return;

	shared<Packet> shared_packet(new Packet());
	shared_packet->append(data, lenght);
	SendEntry(id, shared_packet);
}

void ClientImpl::SendEntry(Connection::id_t id, shared<Packet> packet)
{
	shared<Packet> shared_packet = packet;
	m_pool->enqueue([this, id, shared_packet]()
	{
		ConnectionImpl& conn = m_connections[id];
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

std::unique_ptr<Event> ClientImpl::PullEvent()
{
	std::lock_guard<std::mutex> lock(m_eventMutex);
	if (m_eventQueue.empty())
		return nullptr;

	std::unique_ptr<Event> ptr = std::move(m_eventQueue.front());
	m_eventQueue.pop();
	return std::move(ptr);
}

Entry::id_t ClientImpl::GetEntriesCount(Connection::id_t id)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return 0;

	return m_connections[id].GetEntriesCount();
}

const Entry& ClientImpl::GetEntry(Connection::id_t id, Entry::id_t entryId)
{
	if (id == Connection::UNASSIGNED_ID || id > CLIENT_MAX_CONNECTIONS)
		return ConnectionImpl::EntryDummy;

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
	return msg.ParseFromArray(m_buffer.data, (int)size);
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
				ServerInfoImpl& si = serverList[pPacket->systemAddress];
				PopulateServerInfo(si, &proto_info);
				PushServerListEvent();
			}
		}
		break;
	case ID_UNCONNECTED_PONG:
	{
		RakNet::TimeMS pingTime;
		RakNet::TimeMS current = RakNet::GetTimeMS();
		stream.Read(pingTime);

		ServerInfoImpl& si = serverList[pPacket->systemAddress];
		si.data.ping = current - pingTime;
		PushServerListEvent();

		Packet tosend;
		tosend << (RakNet::MessageID) ID_SERVER_INFO_REQUEST;
		m_peer->AdvertiseSystem(
			pPacket->systemAddress.ToString(false),
			pPacket->systemAddress.GetPort(),
			(const char*)tosend.getData(),
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

		std::unique_ptr<ErrorEvent> ev = std::unique_ptr<ErrorEvent>(new ErrorEvent());
		ev->which = (ErrorId)errorCode;
		{
			std::lock_guard<std::mutex> lock(m_eventMutex);
			m_eventQueue.push(std::move(ev));
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
			ServerInfoImpl& si = serverList[pPacket->systemAddress];
			PopulateServerInfo(si, &proto_info);
			if (si.id != Connection::UNASSIGNED_ID)
			{
				ConnectionImpl& sc = m_connections[si.id];
				sc.data.name = proto_info.name();
				PushConnectionsEvent(si.id, ConnectionEvent::TYPE_NAME_UPDATED);
			}

			PushServerListEvent();
		}

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
			ServerInfoImpl& si = serverList[pPacket->systemAddress];
			BIRIBIT_ASSERT(si.id != Connection::UNASSIGNED_ID);
			ConnectionImpl& sc = m_connections[si.id];

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

			sc.PushServerStatusEvent();
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
			UpdateRemoteClient(pPacket->systemAddress, &proto_client, UPDATE_CLIENT);
		break;
	}
	case ID_CLIENT_DISCONNECTED:
	{
		Proto::Client proto_client;
		if (ReadMessage(proto_client, stream))
			UpdateRemoteClient(pPacket->systemAddress, &proto_client, UPDATE_DISCONNECTION);
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
			ServerInfoImpl& si = serverList[pPacket->systemAddress];
			BIRIBIT_ASSERT(si.id != Connection::UNASSIGNED_ID);
			ConnectionImpl& sc = m_connections[si.id];

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

			sc.PushRoomListEvent();
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
			ServerInfoImpl& si = serverList[pPacket->systemAddress];
			BIRIBIT_ASSERT(si.id != Connection::UNASSIGNED_ID);
			ConnectionImpl& sc = m_connections[si.id];
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

			std::unique_ptr<JoinedRoomEvent> entr(new JoinedRoomEvent());
			entr->connection = si.id;
			entr->room_id = sc.joinedRoom;
			entr->slot_id = sc.joinedSlot;
			{
				std::lock_guard<std::mutex> lock(m_eventMutex);
				m_eventQueue.push(std::move(entr));
			}
		}
		break;
	}
	case ID_SEND_BROADCAST_TO_ROOM:
		BIRIBIT_WARN("Nothing to do with ID_SEND_BROADCAST_TO_ROOM");
		break;
	case ID_BROADCAST_FROM_ROOM:
	{
		ServerInfoImpl& si = serverList[pPacket->systemAddress];
		if (si.id != Connection::UNASSIGNED_ID)
		{
			ConnectionImpl& sc = m_connections[si.id];
			std::unique_ptr<BroadcastEvent> recv(new BroadcastEvent());

			recv->connection = si.id;
			recv->room_id = sc.joinedRoom;
			stream.Read(recv->slot_id);

			if (timeStamp != 0)
				recv->when = timeStamp;

			std::size_t size = BITS_TO_BYTES(stream.GetNumberOfUnreadBits());
			m_buffer.Ensure(size);
			stream.Read(m_buffer.data, size);
			recv->data.append(m_buffer.data, size);

			{
				std::lock_guard<std::mutex> lock(m_eventMutex);
				m_eventQueue.push(std::move(recv));
			}
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
			ServerInfoImpl& si = serverList[pPacket->systemAddress];
			if (si.id != Connection::UNASSIGNED_ID)
			{
				ConnectionImpl& sc = m_connections[si.id];
				unique<Proto::RoomEntriesRequest> proto_entriesReq = sc.UpdateEntries(&proto_entries);
				if (proto_entriesReq != nullptr)
				{
					Proto::RoomEntriesRequest* proto_entriesReqPtr = proto_entriesReq.release();
					RakNet::BitStream bstream;
					WriteMessage(bstream, ID_JOURNAL_ENTRIES_REQUEST, *proto_entriesReqPtr);
					m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, sc.addr, false);
					proto_entriesReq = unique<Proto::RoomEntriesRequest>(proto_entriesReqPtr);
				}

				std::unique_ptr<EntriesEvent> entr(new EntriesEvent());
				entr->connection = si.id;
				entr->room_id = sc.joinedRoom;
				{
					std::lock_guard<std::mutex> lock(m_eventMutex);
					m_eventQueue.push(std::move(entr));
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
			ConnectionImpl& sc = m_connections[i];
			sc.addr = addr;
			sc.data.id = i;
			break;
		}
	}

	if (i == m_connections.size())
		return;

	ServerInfoImpl& si = serverList[addr];
	si.id = i;

	SendProtocolMessageID(ID_SERVER_INFO_REQUEST, addr);
	SendProtocolMessageID(ID_SERVER_STATUS_REQUEST, addr);

	PushConnectionsEvent(si.id, ConnectionEvent::TYPE_NEW_CONNECTION);
}

void ClientImpl::DisconnectFrom(RakNet::SystemAddress addr)
{
	ServerInfoImpl& si = serverList[addr];
	BIRIBIT_ASSERT(si.id != Connection::UNASSIGNED_ID);

	Connection::id_t id = si.id;
	ConnectionImpl& sc = m_connections[id];
	if (si.valid)
		printLog("Disconnected from %s.", sc.data.name.c_str());

	sc.Clear();
	si.id = Connection::UNASSIGNED_ID;
	PushConnectionsEvent(id, ConnectionEvent::TYPE_DISCONNECTION);
}

void ClientImpl::UpdateRemoteClient(RakNet::SystemAddress addr, const Proto::Client* proto_client, TypeUpdateRemoteClient type)
{
	ServerInfoImpl& si = serverList[addr];
	BIRIBIT_ASSERT(si.id != Connection::UNASSIGNED_ID);

	ConnectionImpl& sc = m_connections[si.id];
	if (proto_client->has_id())
	{
		std::uint32_t id = proto_client->id();
		if (id >= sc.clients.size())
			sc.clients.resize(id + 1);

		bool self = proto_client->has_self() && proto_client->self();
		auto ev = std::unique_ptr<RemoteClientEvent>(new RemoteClientEvent());
		ev->connection = si.id;
		
		switch (type)
		{
		case UPDATE_CLIENT:
			PopulateRemoteClient(sc.clients[id], proto_client);
			if (self) sc.selfId = id;
			ev->client = sc.clients[id];
			ev->self = self;
			ev->type = RemoteClientEvent::TYPE_CLIENT_UPDATED;
			break;
		case UPDATE_DISCONNECTION:
			ev->client = sc.clients[id];
			ev->self = (sc.selfId == id);
			ev->type = RemoteClientEvent::TYPE_CLIENT_DISCONNECTED;
			sc.clients[id].id = RemoteClient::UNASSIGNED_ID;
			sc.selfId = ev->self ? RemoteClient::UNASSIGNED_ID : sc.selfId;
			break;
		}

		PushEvent(std::move(ev));
	}
}

void ClientImpl::UpdateRoom(RakNet::SystemAddress addr, const Proto::Room* proto_room)
{
	ServerInfoImpl& si = serverList[addr];
	BIRIBIT_ASSERT(si.id != Connection::UNASSIGNED_ID);
	ConnectionImpl& sc = m_connections[si.id];
	if (proto_room->has_id())
	{
		std::uint32_t id = proto_room->id();
		if (id >= sc.rooms.size())
			sc.rooms.resize(id + 1);

		PopulateRoom(sc.rooms[id], proto_room);
		sc.PushRoomListEvent();
	}
}

void ClientImpl::PopulateServerInfo(ServerInfoImpl& si, const Proto::ServerInfo* proto_info)
{
	if (proto_info->has_name()){
		si.data.name = proto_info->name();
		si.valid = true;
	}

	if (proto_info->has_password_protected())
		si.data.passwordProtected = proto_info->password_protected();

	if (proto_info->has_max_clients())
		si.data.max_clients = proto_info->max_clients();

	if (proto_info->has_connected_clients())
		si.data.connected_clients = proto_info->connected_clients();
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

void ClientImpl::UpdateServerList(std::vector<ServerInfo>& vect)
{
	vect.clear();

	for (auto it = serverList.begin(); it != serverList.end(); it++)
	{
		if (it->second.valid)
		{
			vect.push_back(it->second.data);
			vect.back().addr = it->first.ToString(false);
			vect.back().port = it->first.GetPort();
		}
	}
}

void ClientImpl::UpdateConnections(std::vector<Connection>& vect)
{
	vect.clear();
	auto it = m_connections.begin(); it++;
	for (; it != m_connections.end(); it++)
	{
		if (!it->isNull()) {
			it->data.ping = m_peer->GetAveragePing(it->addr);
			vect.push_back(it->data);
		}
	}
}

void ClientImpl::PushServerListEvent()
{
	PushEvent(std::unique_ptr<ServerListEvent>(new ServerListEvent()));
}

void ClientImpl::PushConnectionsEvent(Connection::id_t id, ConnectionEvent::EventType type)
{
	auto ev = std::unique_ptr<ConnectionEvent>(new ConnectionEvent());
	ConnectionImpl& sc = m_connections[id];
	sc.data.ping = m_peer->GetAveragePing(sc.addr);
	ev->connection = sc.data;
	ev->type = type;

	PushEvent(std::move(ev));
}

} // namespace Biribit