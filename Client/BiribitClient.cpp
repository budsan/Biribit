#include "BiribitClient.h"
#include "TaskPool.h"
#include "PrintLog.h"
#include "Packet.h"

#include <map>
#include <vector>
#include <array>
#include <functional>

#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "PacketLogger.h"
#include "RakNetTypes.h"
#include "StringCompressor.h"
#include "GetTime.h"

#include <BiribitMessageIdentifiers.h>

#include "debug.h"
#include "RefSwap.h"
#include "TaskPool.h"
#include <Types.h>
#include <Generic.h>

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

	ServerConnectionPriv();

	bool isNull();
};

ServerConnectionPriv::ServerConnectionPriv()
	: addr(RakNet::UNASSIGNED_SYSTEM_ADDRESS)
{
}

bool ServerConnectionPriv::isNull()
{
	return addr == RakNet::UNASSIGNED_SYSTEM_ADDRESS;
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
	const std::vector<ServerInfo>& GetDiscoverInfo();
	const std::vector<ServerConnection>& GetConnections();

private: 
	RakNet::RakPeerInterface *m_peer;
	unique<TaskPool> m_pool;

	bool WriteMessage(RakNet::BitStream& bstream, RakNet::MessageID msgId, ::google::protobuf::MessageLite& msg);
	template<typename T> bool ReadMessage(T& msg, Packet& packet);

	static void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
	void RakNetUpdated();
	void HandlePacket(RakNet::Packet*);

	void ConnectedAt(RakNet::SystemAddress);
	void DisconnectFrom(RakNet::SystemAddress);

	void PopulateServerInfo(ServerInfoPriv&, Proto::ServerInfo*);

	Generic::TempBuffer m_buffer;
	
	std::map<RakNet::SystemAddress, ServerInfoPriv> serverList;
	RefSwap<std::vector<ServerInfo>> serverListReq;

	std::array<ServerConnectionPriv, CLIENT_MAX_CONNECTIONS+1> m_connections;
	RefSwap<std::vector<ServerConnection>> connectionsListReq;
	RakNet::TimeMS m_lastDirtyDueTime;

	//std::array<RefSwap<std::vector<RemoteClient>>, CLIENT_MAX_CONNECTIONS+1> m_remoteClientsReq;
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
		while (m_peer != nullptr && (p = m_peer->Receive()) != nullptr)
			HandlePacket(p);

		RakNet::TimeMS current = RakNet::GetTimeMS();
		if ((current - m_lastDirtyDueTime) > 1000)
			connectionsListReq.set_dirty();
	});
}

void ClientImpl::Connect(const char* addr, unsigned short port, const char* password)
{
	if (port == 0)
		port = SERVER_DEFAULT_PORT;

	static const char localhost[] = "localhost";
	if (addr == nullptr)
		addr = localhost;

	m_pool->enqueue([this, addr, port, password]()
	{
		RakNet::ConnectionAttemptResult car = m_peer->Connect(addr, port, password, password != nullptr ? (int) strlen(password) : 0);
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
		for (auto it = ++serverList.begin(); it != serverList.end(); it++)
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
		auto it = serverList.begin();
		while (it != serverList.end())
		{
			if (it->second.id == ServerConnection::UNASSIGNED_ID)
				it = serverList.erase(it);
			else
				it++;
		}
	});
}

const std::vector<ServerInfo>& ClientImpl::GetDiscoverInfo()
{
	if (serverListReq.request())
	{
		m_pool->enqueue([this]()
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
		});
	}

	return serverListReq.front();
}

const std::vector<ServerConnection>& ClientImpl::GetConnections()
{
	if (connectionsListReq.request())
	{
		m_pool->enqueue([this]()
		{
			std::vector<ServerConnection>& back = connectionsListReq.back();
			back.clear();

			for (auto it = ++m_connections.begin(); it != m_connections.end(); it++)
			{
				if (!it->isNull()) {
					it->data.ping = m_peer->GetAveragePing(it->addr);
					back.push_back(it->data);
				}
			}

			connectionsListReq.swap();
			RakNet::TimeMS m_lastDirtyDueTime = RakNet::GetTimeMS();
		});
	}

	return connectionsListReq.front();
}

bool ClientImpl::WriteMessage(RakNet::BitStream& bstream, RakNet::MessageID msgId, ::google::protobuf::MessageLite& msg)
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

void ClientImpl::HandlePacket(RakNet::Packet* pPacket)
{
	Packet packet;
	packet.append(static_cast<const void*>(pPacket->data), pPacket->length);

	RakNet::MessageID packetIdentifier;
	RakNet::Time timeStamp = 0;

	packet >> packetIdentifier;
	if (packetIdentifier == ID_TIMESTAMP)
	{
		RakAssert(pPacket->length > (sizeof(RakNet::MessageID) + sizeof(RakNet::Time)));
		packet.read(&timeStamp, sizeof(timeStamp));
		packet >> packetIdentifier;
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
		packet >> packetIdentifier;
		if (ID_SERVER_INFO_RESPONSE)
		{
			Proto::ServerInfo proto_info;
			if (ReadMessage(proto_info, packet))
			{
				ServerInfoPriv& si = serverList[pPacket->systemAddress];
				PopulateServerInfo(si, &proto_info);
				serverListReq.set_dirty();
			}
		}
		break;
	case ID_UNCONNECTED_PONG:
	{
		RakNet::TimeMS pingTime;
		RakNet::TimeMS current = RakNet::GetTimeMS();
		{
			std::uint32_t temp;
			packet >> temp;
			pingTime = temp;
		}

		ServerInfoPriv& si = serverList[pPacket->systemAddress];
		si.data.ping = current - pingTime;
		serverListReq.set_dirty();
		
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
	case ID_SERVER_INFO_REQUEST:
		BIRIBIT_WARN("Nothing to do with ID_SERVER_INFO_REQUEST");
		break;
	case ID_SERVER_INFO_RESPONSE:
	{
		Proto::ServerInfo proto_info;
		if (ReadMessage(proto_info, packet))
		{
			ServerInfoPriv& si = serverList[pPacket->systemAddress];
			PopulateServerInfo(si, &proto_info);
			if (si.id != ServerConnection::UNASSIGNED_ID)
			{
				ServerConnectionPriv& sc = m_connections[si.id];
				sc.data.name = proto_info.name();
				connectionsListReq.set_dirty();
			}

			serverListReq.set_dirty();
		}

		break;
	}
	case ID_SERVER_STATUS_RESPONSE:
		//TODO
		break;
	case ID_CLIENT_UPDATE_STATUS:
		BIRIBIT_WARN("Nothing to do with ID_SERVER_STATUS_RESPONSE");
		break;
	case ID_CLIENT_NAME_IN_USE:
		//TODO
		break;
	case ID_CLIENT_STATUS_UPDATED:
		//TODO
		break;
	case ID_CLIENT_DISCONNECTED:
		//TODO
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

	Packet tosend;
	tosend << (RakNet::MessageID) ID_SERVER_INFO_REQUEST;
	m_peer->Send((const char*)tosend.getData(),
		(int)tosend.getDataSize(),
		LOW_PRIORITY, RELIABLE_SEQUENCED,
		0, addr, false);

	connectionsListReq.set_dirty();
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
		si.id = ServerConnection::UNASSIGNED_ID;
		connectionsListReq.set_dirty();
	}
}

void ClientImpl::PopulateServerInfo(ServerInfoPriv& si, Proto::ServerInfo* proto_info)
{
	si.data.name = proto_info->name();
	si.valid = true;
	si.data.passwordProtected = proto_info->password_protected();
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

const std::vector<ServerInfo>& Client::GetDiscoverInfo()
{
	return m_impl->GetDiscoverInfo();
}

const std::vector<ServerConnection>& Client::GetConnections()
{
	return m_impl->GetConnections();
}

}