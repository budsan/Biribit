#include "BiribitClient.h"
#include "TaskPool.h"
#include "PrintLog.h"
#include "Packet.h"

ServerInfo::ServerInfo() :
	addr(RakNet::UNASSIGNED_SYSTEM_ADDRESS),
	ping(0), name(), valid (false)
{

}

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

BiribitClient::BiribitClient() : m_peer(nullptr), serverListReqReady(true)
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

BiribitClient::~BiribitClient()
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

void BiribitClient::RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data)
{
	if (data != nullptr && static_cast<BiribitClient*>(data)->m_peer == peer) {
		static_cast<BiribitClient*>(data)->RakNetUpdated();
	}
}

void BiribitClient::RakNetUpdated()
{
	m_pool->enqueue([this]() {
		RakNet::Packet* p = nullptr;
		while (m_peer != nullptr && (p = m_peer->Receive()) != nullptr)
			HandlePacket(p);
	});
}

void BiribitClient::Connect(const char* addr, unsigned short port, const char* password)
{
	if (port == 0)
		port = SERVER_DEFAULT_PORT;

	static const char localhost[] = "localhost";
	if (addr == nullptr)
		addr = localhost;

	m_pool->enqueue([this, addr, port, password]()
	{
		RakNet::ConnectionAttemptResult car = m_peer->Connect(addr, port, password, password != nullptr ? strlen(password) : 0);
		if (car != RakNet::CONNECTION_ATTEMPT_STARTED)
			printLog("Connect failed: %s", ConnectionAttemptResultStr[car]);
	});
}

void BiribitClient::Disconnect()
{
	m_pool->enqueue([this]()
	{
		RakNet::RakNetGUID guid = m_peer->GetGUIDFromIndex(0);
		if (guid != RakNet::UNASSIGNED_RAKNET_GUID)
			m_peer->CloseConnection(guid, true);
	});
}

void BiribitClient::DiscoverOnLan(unsigned short port)
{
	if (port == 0)
		port = SERVER_DEFAULT_PORT;

	m_pool->enqueue([this, port]() {
		m_peer->Ping("255.255.255.255", port, false);
	});
}

void BiribitClient::RefreshDiscoverInfo()
{
	m_pool->enqueue([this]()
	{
		for (auto it = serverList.begin(); it != serverList.end(); it++)
		{
			ServerInfo& info = it->second;
			m_peer->Ping(info.addr.ToString(false), info.addr.GetPort(), false);
			it->second.valid = false;
		}
	});
}

const std::vector<ServerInfo>& BiribitClient::GetDiscoverInfo()
{
	if (serverListReqReady)
	{
		serverListReqReady = false;
		m_pool->enqueue([this]()
		{
			std::vector<ServerInfo>& back = serverListReq.back();
			back.clear();

			for (auto it = serverList.begin(); it != serverList.end(); it++)
			{
				if (it->second.valid)
				{
					back.push_back(it->second);
					back.back().addr = it->first;
				}
			}

			serverListReq.swap();
			serverListReqReady = true;
		});
	}

	return serverListReq.front();
}


void BiribitClient::HandlePacket(RakNet::Packet* pPacket)
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
		break;
	case ID_ALREADY_CONNECTED:
		printLog("ALREADY CONNECTED");
		break;

	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		printLog("ID_INCOMPATIBLE_PROTOCOL_VERSION");
		break;
	case ID_ADVERTISE_SYSTEM:
		printLog("ID_ADVERTISE_SYSTEM");
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

		ServerInfo& info = serverList[pPacket->systemAddress];
		info.ping = current-pingTime;

		break;
	}
	case ID_REMOTE_DISCONNECTION_NOTIFICATION:
		printLog("ID_REMOTE_DISCONNECTION_NOTIFICATION");
		break;
	case ID_REMOTE_CONNECTION_LOST:
		printLog("ID_REMOTE_CONNECTION_LOST");
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION:
		printLog("ID_REMOTE_NEW_INCOMING_CONNECTION");
		break;
	case ID_CONNECTION_BANNED:
		printLog("We are banned from this server.");
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
		printLog("ID_CONNECTION_LOST");
		break;
	case ID_CONNECTION_REQUEST_ACCEPTED:
		printLog("ID_CONNECTION_REQUEST_ACCEPTED");
		break;
	case ID_USER_PACKET_ENUM:
		printLog("ID_USER_PACKET_ENUM");
		break;
	default:
		printLog("UNKNOWN PACKET IDENTIFIER");
		break;
	}
}
