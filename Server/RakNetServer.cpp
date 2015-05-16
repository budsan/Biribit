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

#include <debug.h>
#include <Generic.h>
#include <BiribitMessageIdentifiers.h>

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

RakNetServer::RakNetServer()
	: m_peer(nullptr)
	, m_clients(1)
{

}

RakNetServer::Client::id_t RakNetServer::NewClient(RakNet::SystemAddress addr)
{
	BIRIBIT_ASSERT(m_clientAddrMap.find(addr) == m_clientAddrMap.end());

	Client::id_t i = 1;
	for (; i < m_clients.size() && m_clients[i] != nullptr; i++);

	if (i < m_clients.size())
		m_clients[i] = unique<Client>(new Client());
	else
		m_clients.push_back(unique<Client>(new Client()));

	m_clients[i]->id = i;
	m_clients[i]->appid.clear();
	m_clients[i]->addr = addr;
	m_clientAddrMap[addr] = i;

	int perm_name = 1;
	while (perm_name != 0)
	{
		using clock = std::chrono::system_clock;
		auto timer = clock::now();
		auto now_c = clock::to_time_t(timer) + perm_name;

		int nameID = now_c % sizeof_string_array(randomNames);
		std::string name = randomNames[nameID] + std::to_string(std::hash<int>()(i+nameID)& 0x7F);

		auto itName = m_clientNameMap.find(name);
		if (itName == m_clientNameMap.end())
		{
			m_clientNameMap[name] = i;
			m_clients[i]->name = name;
			perm_name = 0;
		}
		else
			perm_name++;
	}

	Proto::Client proto_client;
	PopulateProtoClient(m_clients[i], &proto_client);
	{
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_CLIENT_STATUS_UPDATED, proto_client))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	}
	{
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_CLIENT_SELF_STATUS, proto_client))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, addr, false);
	}

	return i;
}

void RakNetServer::RemoveClient(RakNet::SystemAddress addr)
{
	auto it = m_clientAddrMap.find(addr);
	BIRIBIT_ASSERT(it != m_clientAddrMap.end());
	BIRIBIT_ASSERT(it->second < m_clients.size());
	BIRIBIT_ASSERT(m_clients[it->second] != nullptr);

	unique<Client>& client = m_clients[it->second];
	Proto::Client proto_client;
	PopulateProtoClient(client, &proto_client);

	if (!client->name.empty())
	{
		std::size_t erased_count = m_clientNameMap.erase(client->name);
		BIRIBIT_ASSERT(erased_count > 0);
	}
	
	m_clients[it->second] = nullptr;
	m_clientAddrMap.erase(it);

	RakNet::BitStream bstream;
	if (WriteMessage(bstream, ID_CLIENT_DISCONNECTED, proto_client))
		m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void RakNetServer::UpdateClient(RakNet::SystemAddress addr, Proto::ClientUpdate* proto_update)
{
	bool updated = false;
	auto itAddr = m_clientAddrMap.find(addr);
	BIRIBIT_ASSERT(itAddr != m_clientAddrMap.end());
	BIRIBIT_ASSERT(itAddr->second < m_clients.size());
	BIRIBIT_ASSERT(m_clients[itAddr->second] != nullptr);
	
	if (proto_update->has_name())
	{
		const std::string& name = proto_update->name();
		auto itName = m_clientNameMap.find(name);
		if (itName != m_clientNameMap.end())
		{
			if (itAddr->second != itName->second)
			{
				// some other client is using that name
				RakNet::BitStream bstream;
				bstream.Write((RakNet::MessageID)ID_CLIENT_NAME_IN_USE);
				m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, addr, false);
			}
		}
		else
		{
			printLog("Client(%d) \"%s\" changed name to \"%s\".", itAddr->second, m_clients[itAddr->second]->name.c_str(), name.c_str());
			m_clientNameMap[name] = itAddr->second;
			m_clients[itAddr->second]->name = name;
			updated = true;
		}
	}

	if (proto_update->has_appid() && m_clients[itAddr->second]->appid != proto_update->appid())
	{
		m_clients[itAddr->second]->appid = proto_update->appid();
		printLog("Client(%d) \"%s\" changed appid to \"%s\".", itAddr->second, m_clients[itAddr->second]->name.c_str(), proto_update->appid().c_str());
		updated = true;
	}

	if (updated)
	{
		Proto::Client proto_client;
		PopulateProtoClient(m_clients[itAddr->second], &proto_client);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_CLIENT_STATUS_UPDATED, proto_client))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	}
}

void RakNetServer::PopulateProtoServerInfo(Proto::ServerInfo* proto_info)
{
	proto_info->set_name(m_name);
	proto_info->set_password_protected(m_passwordProtected);
}

void RakNetServer::PopulateProtoClient(unique<Client>& client, Proto::Client* proto_client)
{
	proto_client->set_id(client->id);
	proto_client->set_name(client->name);
	proto_client->set_appid(client->appid);
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
		printLog("Client %s disconnected.", p->systemAddress.ToString());
		RemoveClient(p->systemAddress);
		break;
	case ID_NEW_INCOMING_CONNECTION:
	{
		Client::id_t id = NewClient(p->systemAddress);
		printLog("New client(%d) \"%s\" connected from %s.", id, m_clients[id]->name.c_str(), p->systemAddress.ToString());
		break;
	}
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		stream.Read(packetIdentifier);
		printLog("%s has Raknet incompatible protocol version. Remote peer version: %d", p->systemAddress.ToString(), packetIdentifier);
		break;
	case ID_ADVERTISE_SYSTEM:
	{
		stream.Read(packetIdentifier);
		if (packetIdentifier == ID_SERVER_INFO_REQUEST)
		{
			Proto::ServerInfo proto_info;
			PopulateProtoServerInfo(&proto_info);
			RakNet::BitStream bstream;
			if (WriteMessage(bstream, ID_SERVER_INFO_RESPONSE, proto_info))
				m_peer->AdvertiseSystem(
					p->systemAddress.ToString(false),
					p->systemAddress.GetPort(),
					(const char*)bstream.GetData(),
					bstream.GetNumberOfBytesUsed());
		}
		break;
	}
	case ID_CONNECTION_LOST:
		printLog("ID_CONNECTION_LOST %s", p->systemAddress.ToString());
		RemoveClient(p->systemAddress);
		break;
	case ID_SERVER_INFO_REQUEST:
	{
		Proto::ServerInfo proto_info;
		PopulateProtoServerInfo(&proto_info);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_SERVER_INFO_RESPONSE, proto_info))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, p->systemAddress, false);
		
		break;
	}
	case ID_SERVER_INFO_RESPONSE:
		BIRIBIT_WARN("Nothing to do with ID_SERVER_INFO_RESPONSE");
		break;
	case ID_CLIENT_SELF_STATUS:
		BIRIBIT_WARN("Nothing to do with ID_CLIENT_SELF_STATUS");
		break;
	case ID_SERVER_STATUS_REQUEST:
	{
		Proto::ServerStatus proto_status;
		for (std::size_t i = 0; i < m_clients.size(); i++)
		{
			if (m_clients[i] != nullptr)
			{
				Proto::Client* proto_client = proto_status.add_clients();
				PopulateProtoClient(m_clients[i], proto_client);
			}
		}

		//TODO: Add Rooms

		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_SERVER_STATUS_RESPONSE, proto_status))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE, 0, p->systemAddress, false);

		break;
	}	
	case ID_SERVER_STATUS_RESPONSE:
		BIRIBIT_WARN("Nothing to do with ID_SERVER_STATUS_RESPONSE");
		break;
	case ID_CLIENT_UPDATE_STATUS:
	{
		Proto::ClientUpdate proto_update;
		ReadMessage(proto_update, stream);
		UpdateClient(p->systemAddress, &proto_update);
		break;
	}	
	case ID_CLIENT_NAME_IN_USE:
		BIRIBIT_WARN("Nothing to do with ID_CLIENT_NAME_IN_USE");
		break;
	case ID_CLIENT_STATUS_UPDATED:
		BIRIBIT_WARN("Nothing to do with ID_SERVER_CLIENT_NAME_CHANGED");
		break;
	case ID_CLIENT_DISCONNECTED:
		BIRIBIT_WARN("Nothing to do with ID_CLIENT_DISCONNECTED");
		break;
	default:
		break;
	}
}

bool RakNetServer::WriteMessage(RakNet::BitStream& bstream,
	RakNet::MessageID msgId,
	::google::protobuf::MessageLite& msg)
{
	std::size_t size = (std::size_t) msg.ByteSize();
	m_buffer.Ensure(size);
	if (msg.SerializeToArray(m_buffer.data, m_buffer.size))
	{
		bstream.Write((RakNet::MessageID) msgId);
		bstream.Write(m_buffer.data, size);
		return true;
	}
	else
		BIRIBIT_WARN("%s unable to serialize.", msg.GetTypeName().c_str());

	return false;
}

template<typename T> bool RakNetServer::ReadMessage(T& msg, RakNet::BitStream& bstream)
{
	std::size_t size = BITS_TO_BYTES(bstream.GetNumberOfUnreadBits());
	m_buffer.Ensure(size);
	bstream.Read(m_buffer.data, size);
	return msg.ParseFromArray(m_buffer.data, size);
}

bool RakNetServer::Run(unsigned short _port, const char* _name, const char* _password)
{
	if (m_peer != nullptr) {
		return true;
	}
		
	m_peer = RakNet::RakPeerInterface::GetInstance();
	m_peer->SetTimeoutTime(10000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);

	m_passwordProtected = (_password != NULL);
	if (m_passwordProtected)
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
