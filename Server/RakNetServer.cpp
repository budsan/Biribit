#include "RakNetServer.h"
#include "Types.h"
#include "PrintLog.h"

#include <sstream>
#include <chrono>

#include "MessageIdentifiers.h"
#include "RakNetStatistics.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "RakSleep.h"
#include "PacketLogger.h"

const char* randomNames[] = {
	"Arianne", "Kesha", "Minerva",
	"Dianna", "Daisey", "Edna",
	"Katerine", "Delpha", "Lucinda",
	"Tierra", "Tyisha", "Zana",
	"Tiffani", "Dorris", "Venice",
	"June", "Lakeisha", "Mafalda",
	"Ardella", "Chantell", "Zaida",
	"Lyn", "Rachael", "Sulema",
	"Raymonde", "Arline", "Martine",
	"Marylyn", "Maribeth", "Genevieve",
	"Lucila", "Sofia", "Madeline",
	"Berneice", "Mozella", "Angelique",
	"Amelia", "Marylynn", "Felice",
	"Tessa", "Sharapova", "Blossom",
	"Keren", "Susan", "Ada",
	"Shenika", "Tandy", "Rebecca",
	"Farah", "Latrisha"
};

template<int N> int sizeof_string_array(const char* (&s)[N]) { return N; }

const unsigned short SERVER_DEFAULT_PORT = 8287;
const unsigned short SERVER_MAX_NUM_CLIENTS = 42;

RakNetServer::RakNetServer() : m_peer(nullptr)
{

}

void RakNetServer::RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data)
{
	if (data != nullptr && static_cast<RakNetServer*>(data)->m_peer == peer) {
		static_cast<RakNetServer*>(data)->RakNetUpdated();
	}
}

void RakNetServer::RakNetUpdated()
{
	m_pool->enqueue([this]() {
		RakNet::Packet* p = nullptr;
		while (m_peer != nullptr && (p = m_peer->Receive()) != nullptr)
			HandlePacket(p);
	});
}

void RakNetServer::HandlePacket(RakNet::Packet* p)
{
	RakNet::BitStream stream(p->data, p->length, false);
	RakNet::MessageID packetIdentifier;
	RakNet::Time timeStamp = 0;

	stream.Read(packetIdentifier);
	if (packetIdentifier == ID_TIMESTAMP)
	{
		RakAssert(p->length > (sizeof(RakNet::MessageID) + sizeof(RakNet::Time)));
		stream.Read(timeStamp);
		stream.Read(packetIdentifier);
	}

	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		m_listener->OnDisconnectionNotification(*this, p->guid.g);
		break;
	case ID_NEW_INCOMING_CONNECTION:
		m_listener->OnNewIncomingConnection(*this, p->guid.g);
		break;
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		stream.Read(packetIdentifier);
		printLog("%s has Raknet incompatible protocol version. Remote peer version: %d", p->systemAddress.ToString(), packetIdentifier);
		break;
	case ID_ADVERTISE_SYSTEM:
	{
		shared<Packet> packet = shared<Packet>(new Packet());
		std::size_t offset = (stream.GetReadOffset() / 8);
		packet->append(p->data + offset, p->length - offset);
		m_listener->OnAdvertisement(*this, p->systemAddress.ToString(), p->systemAddress.GetPort(), packet);
		break;
	}
	case ID_CONNECTION_LOST:
		m_listener->OnConnectionLost(*this, p->guid.g);
		break;
	case ID_USER_PACKET_ENUM:
	{
		shared<Packet> packet = shared<Packet>(new Packet());
		std::size_t offset = (stream.GetReadOffset() / 8);
		packet->append(p->data + offset, p->length - offset);
		m_listener->OnPacket(*this, packet);
		break;
	}	
	default:
		break;
	}
}

bool RakNetServer::Run(shared<ServerListener> _listener, unsigned short _port, const char* _name, const char* _password)
{
	if (m_peer != nullptr) {
		return true;
	}
		
	m_peer = RakNet::RakPeerInterface::GetInstance();
	m_peer->SetTimeoutTime(10000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);

	if (_password != NULL)
		m_peer->SetIncomingPassword(_password, (int)strlen(_password));

	if (_port == 0)
		_port = SERVER_DEFAULT_PORT;

	printLog("Starting server. Server port: %d", _port);

	RakNet::SocketDescriptor socketDescriptors[2];
	socketDescriptors[0].port = _port;
	socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4
	socketDescriptors[1].port = _port;
	socketDescriptors[1].socketFamily = AF_INET6; // Test out IPV6

	bool bOk = m_peer->Startup(SERVER_MAX_NUM_CLIENTS, socketDescriptors, 2) == RakNet::RAKNET_STARTED;
	m_peer->SetMaximumIncomingConnections(SERVER_MAX_NUM_CLIENTS);
	if (!bOk)
	{
		printLog("Failed to start dual IPV4 and IPV6 ports. Trying IPV4 only.");

		bool bOk = m_peer->Startup(SERVER_MAX_NUM_CLIENTS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
		if (!bOk)
		{
			printLog("Server failed to start.  Terminating.");
			Close();
			return false;
		}
		else
		{
			printLog("IPV4 only success.");
		}
	}

	m_peer->SetOccasionalPing(true);
	m_peer->SetUnreliableTimeout(1000);

	std::stringstream log;
	m_peer->GetLocalIP(0);
	log << "Network interface(s) detected: " << m_peer->GetNumberOfAddresses() << std::endl;
	for (unsigned int i = 0; i < m_peer->GetNumberOfAddresses(); i++) {
		RakNet::SystemAddress sa = m_peer->GetInternalID(RakNet::UNASSIGNED_SYSTEM_ADDRESS, i);
		log << " " << i + 1 << ". " << sa.ToString(false) << " (LAN=" << sa.IsLANAddress() << ")" << std::endl;
	}
	printLog(log.str().c_str());

	if (_name == NULL) 
	{
		using clock = std::chrono::system_clock;
		auto timer = clock::now();
		auto now_c = clock::to_time_t(timer);

		int nameID = now_c % sizeof_string_array(randomNames);
		m_name = randomNames[nameID];
	}
	else
	{
		m_name = _name;
	}

	m_listener = _listener;
	m_pool = std::unique_ptr<TaskPool>(new TaskPool(1, "RakNetServer"));
	m_peer->SetUserUpdateThread(RaknetThreadUpdate, this);

	printLog("Server \"%s\" running successfully", m_name.c_str());
	return true;
}

bool RakNetServer::isRunning()
{
	return m_peer != nullptr;
}

bool RakNetServer::Close()
{
	if (m_peer != nullptr)
	{
		m_peer->SetUserUpdateThread(RaknetThreadUpdate, nullptr);

		if (m_peer->IsActive()) {
			printLog("Server \"%s\" shutting down...", m_name.c_str());
			m_peer->Shutdown(60000);
		}
			
		printLog("Waiting for thread ends...");
		m_pool.reset(nullptr);

		RakNet::RakPeerInterface::DestroyInstance(m_peer);
		m_peer = nullptr;

		return true;
	}

	return false;
}

void RakNetServer::Send(std::uint64_t id, shared<Packet> packet)
{
	if (m_peer == nullptr)
		return;

	m_pool->enqueue([this, id, packet]() 
	{
		m_peer->Send(
			(const char*)packet->getData(),
			(int)packet->getDataSize(),
			LOW_PRIORITY, RELIABLE_SEQUENCED,
			0, RakNet::RakNetGUID(id), false);
	});
}

void RakNetServer::AdvertiseSystem(const std::string &addr, unsigned short port, shared<Packet> packet)
{
	if (m_peer == nullptr)
		return;

	m_pool->enqueue([this, addr, port, packet]()
	{
		m_peer->AdvertiseSystem(addr.c_str(), port, 
			(const char*)packet->getData(),
			(int)packet->getDataSize());
	});
}
