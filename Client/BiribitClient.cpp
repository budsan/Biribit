#include "BiribitClient.h"
#include "TaskPool.h"
#include "PrintLog.h"
#include "Packet.h"

#include <map>
#include <vector>
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

#include "RefSwap.h"
#include "TaskPool.h"
#include <Types.h>
#include <Generic.h>

ServerDescription::ServerDescription()
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

struct ServerDescriptionPriv
{
	ServerDescription data;
	ServerConnection::id_t id;
	bool valid;

	ServerDescriptionPriv();
};

ServerDescriptionPriv::ServerDescriptionPriv()
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

	bool Null();
};

ServerConnectionPriv::ServerConnectionPriv()
	: addr(RakNet::UNASSIGNED_SYSTEM_ADDRESS)
{
}

bool ServerConnectionPriv::Null()
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

const unsigned short SERVER_DEFAULT_PORT = 8287;

class BiribitClientImpl
{
public:
	BiribitClientImpl();
	~BiribitClientImpl();

	void Connect(const char* addr = nullptr, unsigned short port = 0, const char* password = nullptr);
	void Disconnect(ServerConnection::id_t id);
	void Disconnect();

	void DiscoverOnLan(unsigned short port);
	void ClearDiscoverInfo();
	void RefreshDiscoverInfo();
	const std::vector<ServerDescription>& GetDiscoverInfo();
	const std::vector<ServerConnection>& GetConnections();

private: 
	RakNet::RakPeerInterface *m_peer;
	unique<TaskPool> m_pool;

	bool WriteMessage(RakNet::BitStream& bstream, RakNet::MessageID msgId, ::google::protobuf::MessageLite& msg);
	template<typename T> bool ReadMessage(T& msg, Packet& packet);

	static void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
	void RakNetUpdated();
	void HandlePacket(RakNet::Packet*);

	void ConnectedAt(RakNet::Packet*);
	void DisconnectFrom(RakNet::Packet*);

	void Rcv_ServerInfoResponse(ServerDescriptionPriv&, ServerInfo&);

	Generic::TempBuffer m_buffer;

	std::map<RakNet::SystemAddress, ServerDescriptionPriv> serverList;
	RefSwap<std::vector<ServerDescription>> serverListReq;
	bool serverListReqReady;

	std::vector<ServerConnectionPriv> m_connections;
	RefSwap<std::vector<ServerConnection>> connectionsListReq;
	bool connectionsListReqReady;
};

BiribitClientImpl::BiribitClientImpl()
	: m_peer(nullptr)
	, serverListReqReady(true)
	, connectionsListReqReady(true)
	, m_connections(1)
{
	m_pool = unique<TaskPool>(new TaskPool(1, "BiribitClient"));

	m_peer = RakNet::RakPeerInterface::GetInstance();
	m_peer->SetUserUpdateThread(RaknetThreadUpdate, this);
	m_peer->AllowConnectionResponseIPMigration(false);
	m_peer->SetOccasionalPing(true);

	RakNet::SocketDescriptor socketDescriptor;
	socketDescriptor.socketFamily = AF_INET;

	RakNet::StartupResult result = m_peer->Startup(1, &socketDescriptor, 1);
	if (result != RakNet::RAKNET_STARTED)
	{
		printLog("Startup failed: %s", StartupResultStr[result]);
		RakNet::RakPeerInterface::DestroyInstance(m_peer);
		m_peer = nullptr;
	}
}

BiribitClientImpl::~BiribitClientImpl()
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

void BiribitClientImpl::RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data)
{
	if (data != nullptr && static_cast<BiribitClientImpl*>(data)->m_peer == peer) {
		static_cast<BiribitClientImpl*>(data)->RakNetUpdated();
	}
}

void BiribitClientImpl::RakNetUpdated()
{
	m_pool->enqueue([this]() {
		RakNet::Packet* p = nullptr;
		while (m_peer != nullptr && (p = m_peer->Receive()) != nullptr)
			HandlePacket(p);
	});
}

void BiribitClientImpl::Connect(const char* addr, unsigned short port, const char* password)
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

void BiribitClientImpl::Disconnect(ServerConnection::id_t id)
{
	m_pool->enqueue([this, id]()
	{
		if (id < m_connections.size() && !m_connections[id].Null())
		{
			m_peer->CloseConnection(m_connections[id].addr, true);
			m_connections[id].addr = RakNet::UNASSIGNED_SYSTEM_ADDRESS;
		}
	});
}

void BiribitClientImpl::Disconnect()
{
	m_pool->enqueue([this]()
	{
		for (std::size_t i = 1; i < m_connections.size(); i++)
		{
			if (!m_connections[i].Null())
			{
				m_peer->CloseConnection(m_connections[i].addr, true);
				m_connections[i].addr = RakNet::UNASSIGNED_SYSTEM_ADDRESS;
			}
		}
	});
}

void BiribitClientImpl::DiscoverOnLan(unsigned short port)
{
	if (port == 0)
		port = SERVER_DEFAULT_PORT;

	m_pool->enqueue([this, port]() {
		printLog("Discovering on Lan in port %d...", port);
		m_peer->Ping("255.255.255.255", port, false);
	});
}

void BiribitClientImpl::RefreshDiscoverInfo()
{
	m_pool->enqueue([this]()
	{
		for (auto it = serverList.begin(); it != serverList.end(); it++)
		{
			ServerDescriptionPriv& info = it->second;
			m_peer->Ping(it->first.ToString(false), it->first.GetPort(), false);
			it->second.valid = false;
		}
	});
}

void BiribitClientImpl::ClearDiscoverInfo()
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

const std::vector<ServerDescription>& BiribitClientImpl::GetDiscoverInfo()
{
	if (serverListReqReady)
	{
		serverListReqReady = false;
		m_pool->enqueue([this]()
		{
			std::vector<ServerDescription>& back = serverListReq.back();
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
			serverListReqReady = true;
		});
	}

	return serverListReq.front();
}

const std::vector<ServerConnection>& BiribitClientImpl::GetConnections()
{
	if (connectionsListReqReady)
	{
		connectionsListReqReady = false;
		m_pool->enqueue([this]()
		{
			std::vector<ServerConnection>& back = connectionsListReq.back();
			back.clear();

			for (auto it = (++m_connections.begin()); it != m_connections.end(); it++)
			{
				if (!it->Null()) {
					back.push_back(it->data);
				}
			}

			connectionsListReq.swap();
			connectionsListReqReady = true;
		});
	}

	return connectionsListReq.front();
}

bool BiribitClientImpl::WriteMessage(RakNet::BitStream& bstream, RakNet::MessageID msgId, ::google::protobuf::MessageLite& msg)
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

template<typename T> bool BiribitClientImpl::ReadMessage(T& msg, Packet& packet)
{
	std::size_t size = packet.getDataSize() - packet.getReadPos();
	m_buffer.Ensure(size);
	packet.read(m_buffer.data, size);
	return msg.ParseFromArray(m_buffer.data, (int) size);
}

void BiribitClientImpl::HandlePacket(RakNet::Packet* pPacket)
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
		printLog("ID_DISCONNECTION_NOTIFICATION");
		DisconnectFrom(pPacket);
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
			ServerInfo msg_si;
			if (ReadMessage(msg_si, packet))
			{
				ServerDescriptionPriv& sd = serverList[pPacket->systemAddress];
				Rcv_ServerInfoResponse(sd, msg_si);
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

		ServerDescriptionPriv& sd = serverList[pPacket->systemAddress];
		sd.data.ping = current - pingTime;
		
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
		DisconnectFrom(pPacket);
		break;
	case ID_REMOTE_CONNECTION_LOST:
		printLog("ID_REMOTE_CONNECTION_LOST");
		DisconnectFrom(pPacket);
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION:
		printLog("ID_REMOTE_NEW_INCOMING_CONNECTION");
		break;
	case ID_CONNECTION_BANNED:
		printLog("Banned from server.");
		DisconnectFrom(pPacket);
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
		DisconnectFrom(pPacket);
		break;
	case ID_CONNECTION_REQUEST_ACCEPTED:
		ConnectedAt(pPacket);
		break;
	case ID_SERVER_INFO_RESPONSE:
		{
			ServerInfo msg_si;
			if (ReadMessage(msg_si, packet))
			{
				ServerDescriptionPriv& sd = serverList[pPacket->systemAddress];
				Rcv_ServerInfoResponse(sd, msg_si);
				if (sd.id != ServerConnection::UNASSIGNED_ID)
				{
					ServerConnectionPriv& sc = m_connections[sd.id];
					sc.data.name = msg_si.name();
				}
			}
		}
		break;
	default:
		printLog("UNKNOWN PACKET IDENTIFIER");
		break;
	}
}

void BiribitClientImpl::ConnectedAt(RakNet::Packet* pPacket)
{
	//Looking for room in m_connections vector
	std::size_t i = 1;
	for (; i < m_connections.size(); i++)
	{
		if (m_connections[i].Null())
		{
			ServerConnectionPriv& sc = m_connections[i];
			sc.addr = pPacket->systemAddress;
			sc.data.id = i;
			break;
		}
	}

	if (i == m_connections.size())
	{
		ServerConnectionPriv sc;
		sc.addr = pPacket->systemAddress;
		sc.data.id = i;
		m_connections.push_back(sc);
	}

	ServerDescriptionPriv& sd = serverList[pPacket->systemAddress];
	sd.id = i;

	Packet tosend;
	tosend << (RakNet::MessageID) ID_SERVER_INFO_REQUEST;
	m_peer->Send((const char*)tosend.getData(),
		(int)tosend.getDataSize(),
		LOW_PRIORITY, RELIABLE_SEQUENCED,
		0, pPacket->systemAddress, false);

	GetConnections();
}

void BiribitClientImpl::DisconnectFrom(RakNet::Packet* pPacket)
{
	ServerDescriptionPriv& sd = serverList[pPacket->systemAddress];
	if (sd.id != ServerConnection::UNASSIGNED_ID)
	{
		ServerConnectionPriv& sc = m_connections[sd.id];
		sc.addr = RakNet::UNASSIGNED_SYSTEM_ADDRESS;
		sd.id = ServerConnection::UNASSIGNED_ID;
	}
}

void BiribitClientImpl::Rcv_ServerInfoResponse(ServerDescriptionPriv& sd, ServerInfo& msg_si)
{
	sd.data.name = msg_si.name();
	sd.valid = true;
	sd.data.passwordProtected = msg_si.password_protected();
}

//---------------------------------------------------------------------------//

BiribitClient::BiribitClient() : m_impl(new BiribitClientImpl())
{

}

BiribitClient::~BiribitClient()
{
	delete m_impl;
}

void BiribitClient::Connect(const char* addr, unsigned short port, const char* password)
{
	m_impl->Connect(addr, port, password);
}

void BiribitClient::Disconnect(ServerConnection::id_t id)
{
	m_impl->Disconnect(id);
}

void BiribitClient::Disconnect()
{
	m_impl->Disconnect();
}

void BiribitClient::DiscoverOnLan(unsigned short port)
{
	m_impl->DiscoverOnLan(port);
}

void BiribitClient::ClearDiscoverInfo()
{
	m_impl->ClearDiscoverInfo();
}

void BiribitClient::RefreshDiscoverInfo()
{
	m_impl->RefreshDiscoverInfo();
}

const std::vector<ServerDescription>& BiribitClient::GetDiscoverInfo()
{
	return m_impl->GetDiscoverInfo();
}

const std::vector<ServerConnection>& BiribitClient::GetConnections()
{
	return m_impl->GetConnections();
}