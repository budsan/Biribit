#include "Server.h"

#include <fstream>
#include <sstream>
#include <random>
#include <ctime>

#include <Config.h>
#include <Packet.h>
#include <PrintLog.h>

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

//---------------------------------------------------------------------------//

bool ReadString(std::string& str, RakNet::BitStream *stream)
{
	brbt::str_size_t length;
	stream->Read(length);

	str.clear();
	for (std::size_t i = 0; i < length; i++)
	{
		char c; stream->Read(c);
		str.push_back(c);
	}

	return length > 0;
}

PacketReliability Server::ParseReliability(PacketReliabilityBitmask mask)
{
	PacketReliability r;
	switch (mask)
	{
	case (Reliable) :
		r = RELIABLE;
		break;
	case (Ordered) :
		r = UNRELIABLE_SEQUENCED;
		break;
	case (Reliable | Ordered) :
		r = RELIABLE_ORDERED;
		break;
	default:
		r = UNRELIABLE;
	}

	return r;
}

//---------------------------------------------------------------------------//

#include "SignalTypes.h"

#define MAX_NUM_CLIENTS 64
#define CONGESTION_TIME_CHECK 5

Server::Server() : peer(nullptr), elapsedTime(0), forceExit(false), peerLastUniqueID(0)
{
	lobbies.push_back(std::unique_ptr<Lobby>());
}

Server::~Server()
{
	Destroy();
}

void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data)
{
	if (data != nullptr)
		((Server*)data)->WakeLoop(peer);
}

bool Server::bInit(const char* password, int iPort, const char* name)
{
	if (peer != nullptr) return true;

	peer = RakNet::RakPeerInterface::GetInstance();

	peer->SetIncomingPassword(password, (int)strlen(password));
	peer->SetTimeoutTime(10000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);

	if (iPort == 0)
		iPort = DEFAULT_SERVER_PORT;

	printLog("Starting server. Server port: %d", iPort);

	RakNet::SocketDescriptor socketDescriptors[2];
	socketDescriptors[0].port = iPort;
	socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4
	socketDescriptors[1].port = iPort;
	socketDescriptors[1].socketFamily = AF_INET6; // Test out IPV6

	bool bOk = peer->Startup(MAX_NUM_CLIENTS, socketDescriptors, 2) == RakNet::RAKNET_STARTED;
	peer->SetMaximumIncomingConnections(MAX_NUM_CLIENTS);
	if (!bOk)
	{
		printLog("Failed to start dual IPV4 and IPV6 ports. Trying IPV4 only.");

		bool bOk = peer->Startup(MAX_NUM_CLIENTS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
		if (!bOk)
		{
			printLog("Server failed to start.  Terminating.");
			Destroy();
			return false;
		}
		else
		{
			printLog("IPV4 only success.");
		}
	}

	peer->SetOccasionalPing(true);
	peer->SetUnreliableTimeout(1000);
	peer->SetUserUpdateThread(RaknetThreadUpdate, this);

	std::stringstream log;
	peer->GetLocalIP(0);
	log << "Network interface(s) detected: " << peer->GetNumberOfAddresses() << std::endl;
	for (unsigned int i = 0; i < peer->GetNumberOfAddresses(); i++) {
		RakNet::SystemAddress sa = peer->GetInternalID(RakNet::UNASSIGNED_SYSTEM_ADDRESS, i);
		log << " " << i + 1 << ". " << sa.ToString(false) << " (LAN=" << sa.IsLANAddress() << ")" << std::endl;
	}
	printLog(log.str().c_str());

	ChangeName(name);

	printLog("Server \"%s\" running successfully", serverName.c_str());

	elapsedTime = 0;
	forceExit = false;

	return true;
}

void Server::ChangeName(const char* name)
{
	if (name == NULL) {
		timer = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(timer);

		int nameID = now_c % sizeof_string_array(randomNames);
		serverName = randomNames[nameID];
	}
	else {
		serverName = name;
	}
}

void Server::Run()
{
	while (peer != nullptr && !forceExit)
	{
		RakNet::Packet* p = peer->Receive();
		if (p == nullptr)
		{
			std::unique_lock<std::mutex> lock(loopMutex);
			loopCondVar.wait(lock);
		}
		else
		{
			HandlePacket(p);
		}

		TimePoint now = std::chrono::system_clock::now();
		std::chrono::duration<double> elapsed = now - timer;
		timer = now;

		elapsedTime += elapsed.count();
		if (!forceExit && elapsedTime >= 1.0f)
		{
			CheckCongestion();
			elapsedTime = 0.0f;
		}

		peer->DeallocatePacket(p);
	}
}

void Server::Close()
{
	if (!forceExit)
	{
		printLog("Closing server...");
		forceExit = true;
		loopCondVar.notify_all();
	}
}

void Server::WakeLoop(RakNet::RakPeerInterface *_peer)
{
	if (peer == _peer)
		loopCondVar.notify_all();
}

void Server::Destroy()
{
	if (peer != nullptr)
	{
		PeerMapIter it = remotePeers.begin();
		for (; it != remotePeers.end(); it++) {
			peer->CloseConnection(it->first, true);
		}


		if (peer->IsActive()) peer->Shutdown(60000);
		RakNet::RakPeerInterface::DestroyInstance(peer);
		peer = nullptr;
	}
}

brbt::id_t Server::bSetPeerName(RakNet::SystemAddress* address, const std::string &name)
{
	if (name.empty()) {
		printLog("-- Cannot set a empty string as a name.");
		return 0;
	}

	PeerMapIter it = remotePeers.find(*address);
	if (it == remotePeers.end())
		return 0;

	std::map<std::string, PeerMapIter>::iterator it2 = peerNames.find(name);
	if (it2 != peerNames.end()) {
		if (it == it2->second) {
			//this peer have the same name already
			return it->second.id;
		}
		else {
			printLog("-- Cannot set \"%s\" to %s, it's in use.", name.c_str(), address->ToString());
			return 0;
		}
	}

	if (!it->second.name.empty()) {
		//peer was named before, we must erase from map before
		peerNames.erase(it->second.name);
	}

	//we keep the name and the reference
	it->second.name = name;
	peerNames.insert(std::pair<std::string, PeerMapIter>(name, it));

	if (it->second.id == 0) {
		it->second.id = ++peerLastUniqueID;
		peerIDs.insert(std::pair<brbt::id_t, PeerMapIter>(peerLastUniqueID, it));
	}

	return it->second.id;
}

Server::PeerMapIter Server::pGetPeer(const RakNet::SystemAddress* address)
{
	return remotePeers.find(*address);
}

bool Server::IsPeerValid(PeerMapIter iter)
{
	return iter != remotePeers.end();
}

void Server::Connected(RakNet::SystemAddress* address)
{
	PeerMapIter it = pGetPeer(address);
	if (!IsPeerValid(it)) {
		remotePeers.insert(PeerPair(*address, Peer()));
	}
}

void Server::Disconnected(RakNet::SystemAddress* address)
{
	PeerMapIter it = remotePeers.find(*address);
	if (it != remotePeers.end())
	{
		if (it->second.id != 0)
		{
			//Notifying rest of the clients this client disconnection
			RakNet::BitStream ToSend;
			ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
			ToSend.Write((brbt::packet_id_t) SERVER_CLIENT_DISCONNECTION);
			ToSend.Write((brbt::id_t) it->second.id);
			peer->Send(&ToSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);

			peerNames.erase(it->second.name);
			peerIDs.erase(it->second.id);
		}

		//Unsuscribe from all lobbies
		auto it2 = it->second.lobbies.begin();
		for (; it2 != it->second.lobbies.end(); it2++) {
			lobbies[*it2]->peers.erase(*address);
		}

		remotePeers.erase(*address);
	}
}

brbt::id_t Server::SubscribeLobby(const std::string &name, PeerMapIter remote)
{
	brbt::id_t& id = lobbiesNames[name];
	if (id == 0) {
		//It's a new service!
		id = (brbt::id_t) lobbies.size();
		lobbies.push_back(LobbyPtr(new Lobby(id, name)));
		printLog("+ New service created: %s", name.c_str());
	}

	lobbies[id]->peers.insert(remote->first);
	remote->second.lobbies.insert(id);

	return id;
}

bool Server::UnsubscribeLobby(brbt::id_t id, PeerMapIter remote)
{
	if (id < (brbt::id_t) lobbies.size()) {
		std::size_t erased = lobbies[id]->peers.erase(remote->first);
		return erased > 0;
	}

	return false;
}

void Server::HandlePacket(RakNet::Packet* p)
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

	PeerMapIter remote = pGetPeer(&p->systemAddress);
	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		if (!IsPeerValid(remote) && !remote->second.name.empty()) {
			printLog("%s (%s) has disconnected from the server.", remote->second.name.c_str(), p->systemAddress.ToString());
		}
		else {
			printLog("%s has disconnected from the server.", p->systemAddress.ToString());
		}

		Disconnected(&p->systemAddress);
		break;
	case ID_NEW_INCOMING_CONNECTION:
	{
		printLog("New incoming connection from %s", p->systemAddress.ToString());
		Connected(&p->systemAddress);

		RakNet::BitStream ToSend;
		ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
		ToSend.Write((brbt::packet_id_t) SERVER_INFO);

		ToSend.Write((brbt::str_size_t) serverName.length());
		ToSend.Write(serverName.c_str(), (unsigned int)serverName.length());

		peer->Send(&ToSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, false);

		break;
	}
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		stream.Read(packetIdentifier);
		printLog("%s has Raknet incompatible protocol version. Remote peer version: %d", p->systemAddress.ToString(), packetIdentifier);
		Disconnected(&p->systemAddress);
		break;
	case ID_ADVERTISE_SYSTEM:
	{
		stream.Read(packetIdentifier);
		switch (packetIdentifier)
		{
		case ID_SERVER_PACKET:
		{
			int serverPackageID;
			stream.Read(serverPackageID);
			switch (serverPackageID)
			{
			case SERVER_INFO_REQUEST:
				RakNet::BitStream ToSend;
				ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
				ToSend.Write((brbt::packet_id_t) SERVER_INFO);

				ToSend.Write((brbt::str_size_t) serverName.length());
				ToSend.Write(serverName.c_str(), (unsigned int)serverName.length());

				peer->AdvertiseSystem(p->systemAddress.ToString(), p->systemAddress.GetPort(), (const char*)ToSend.GetData(), ToSend.GetNumberOfBytesUsed());
				break;
			}
			break;
		}
		}
		break;
	}
	case ID_CONNECTION_LOST:
		if (!IsPeerValid(remote) && !remote->second.name.empty())
			printLog("Connection lost from the server: %s %s", remote->second.name.c_str(), p->systemAddress.ToString());
		else
			printLog("Connection lost from the server: %s", p->systemAddress.ToString());

		Disconnected(&p->systemAddress);
		break;
	case ID_SERVER_PACKET:
		HandleServerPacket(p, &stream, remote);
		break;
	case ID_SERVICE_PACKET:
		HandleLobbyPacket(p, &stream, remote);
		break;
	case ID_SERVICE_PACKET_WITH_RELIABILITY:
	{
		unsigned char mask;
		stream.Read(mask);
		HandleLobbyPacket(p, &stream, remote, ParseReliability((PacketReliabilityBitmask)mask));
		break;
	}
	default:

		break;
	}
}

void Server::HandleLobbyPacket(RakNet::Packet* p, RakNet::BitStream *stream, PeerMapIter remote, PacketReliability reliability)
{
	if (!IsPeerValid(remote) || remote->second.id == 0) {
		printLog("UNAUTHORIZED %s REMOTE PEER. PACKET AVOIDED.", p->systemAddress.ToString());
		return;
	}

	brbt::id_t clientId;
	stream->Read(clientId);

	if (clientId != remote->second.id) {
		printLog("CLIENT ID DOESN'T MATCH. SENT: %d. EXPECTED %d. PACKET AVOIDED.", p->systemAddress.ToString());
	}

	brbt::id_t id;
	stream->Read(id);
	PacketPriority priority = MEDIUM_PRIORITY;

	if (id == 0 || id >= (brbt::id_t) lobbies.size()) {
		printLog("UNKNOWN SERVICE: %d FROM %s REMOTE PEER. PACKET AVOIDED.", id, p->systemAddress.ToString());
		return;
	}

	std::set<RakNet::SystemAddress>& addrs = lobbies[id]->peers;
	for (auto it = addrs.begin(); it != addrs.end(); ++it)
	{
		if (*it != p->systemAddress)
			peer->Send((const char*)p->data, p->length, priority, reliability, (char)(id & 0xFF), *it, false);
	}
}

void Server::HandleServerPacket(RakNet::Packet* p, RakNet::BitStream *stream, PeerMapIter remote)
{
	int serverPackageID;
	stream->Read(serverPackageID);

	switch (serverPackageID) {
	case SERVER_SET_NAME:
	{
		static std::string name;
		ReadString(name, stream);

		printLog("+ %s's name is \"%s\"", p->systemAddress.ToString(), name.c_str());

		brbt::id_t id = bSetPeerName(&p->systemAddress, name);
		if (id > 0)
		{
			// SEND TO PEER NAME CONFIRMATION AND LIST ALL USERS
			{
				RakNet::BitStream ToSend;
				ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
				ToSend.Write((brbt::packet_id_t) SERVER_SET_NAME_SUCCESS);

				ToSend.Write((brbt::id_t) id);

				ToSend.Write((brbt::str_size_t) name.length());
				ToSend.Write(name.c_str(), (unsigned int)name.length());

				ToSend.Write((brbt::str_size_t) remotePeers.size());
				PeerMapIter it = remotePeers.begin();
				for (; it != remotePeers.end(); it++)
				{
					if (it->second.id == 0) continue;
					ToSend.Write((brbt::id_t) it->second.id);

					std::string &peername = it->second.name;
					ToSend.Write((brbt::str_size_t) peername.length());
					ToSend.Write(peername.c_str(), (unsigned int)peername.length());
				}

				peer->Send(&ToSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, false);
			}

			{
				// SEND TO EVERYBODY THAT A NEW PEER HAS CONNECTED AND CONFIRMED ITS NAME
				RakNet::BitStream ToSend;
				ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
				ToSend.Write((brbt::packet_id_t) SERVER_CLIENT_CONNECTION);

				ToSend.Write((brbt::id_t) id);
				ToSend.Write((brbt::str_size_t) name.length());
				ToSend.Write(name.c_str(), (unsigned int)name.length());

				peer->Send(&ToSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
			}
		}
		else
		{
			// SEND TO PEER SET NAME ERROR
			RakNet::BitStream ToSend;
			ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
			ToSend.Write((brbt::packet_id_t) SERVER_SET_NAME_ERROR);
			peer->Send(&ToSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, false);
		}
	}
	break;
	case SERVER_CONGESTION:
		printLog("- SERVER_CONGESTION");
	case SERVER_SET_NAME_ERROR:
		printLog("- SET_NAME_ERROR SERVER");
	case SERVER_SET_NAME_SUCCESS:
		printLog("- SERVER_SET_NAME_SUCCESS. THIS PACKET IS RESERVED FOR SERVER USE ONLY.");
		break;
	case SERVER_SERVICE_SUBSCRIBE:
	{
		static std::string service;
		ReadString(service, stream);

		if (!service.empty())
		{
			brbt::id_t id = SubscribeLobby(service, remote);

			{
				RakNet::BitStream ToSend;
				ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
				ToSend.Write((brbt::packet_id_t) SERVER_SERVICE_SUBSCRIBED);
				ToSend.Write((brbt::id_t) id);

				ToSend.Write((brbt::str_size_t) service.length());
				ToSend.Write(service.c_str(), (unsigned int)service.length());

				std::set<RakNet::SystemAddress>& addrs = lobbies[id]->peers;
				for (auto it = addrs.begin(); it != addrs.end(); ++it)
				{
					if (*it != p->systemAddress) {
						PeerMapIter peer = pGetPeer(&(*it));
						ToSend.Write((brbt::id_t) peer->second.id);
					}
				}

				peer->Send(&ToSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, false);
			}

			{
				RakNet::BitStream ToSend;
				ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
				ToSend.Write((brbt::packet_id_t) SERVER_CLIENT_SUBSCRIBED);
				ToSend.Write((brbt::id_t) remote->second.id); //client id
				ToSend.Write((brbt::id_t) id); //service id

				peer->Send(&ToSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
			}
		}

		break;
	}
	case SERVER_SERVICE_UNSUBSCRIBE:
	{
		brbt::id_t id;
		stream->Read(id);
		if (UnsubscribeLobby(id, remote))
		{
			RakNet::BitStream ToSend;
			ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
			ToSend.Write((brbt::packet_id_t) SERVER_CLIENT_UNSUBSCRIBED);
			ToSend.Write((brbt::id_t) remote->second.id); //client id
			ToSend.Write((brbt::id_t) id); //service id

			peer->Send(&ToSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
		}
		break;
	}
	default:
		printLog("- UNKNOWN SERVER PACKET %d FROM %s", serverPackageID, p->systemAddress.ToString());
		break;
	}
}

void Server::CheckCongestion()
{
	bool stopped = false;
	RakNet::RakNetStatistics stats;
	std::set<brbt::id_t> lobbiesCongestioned;

	for (PeerMapIter it = remotePeers.begin(); it != remotePeers.end(); it++)
	{
		PeerMapIter remote = pGetPeer(&(it->first));
		RakNet::RakNetStatistics *res = peer->GetStatistics(it->first, &stats);
		if (res == nullptr) continue;

		double bufferedBytesThisTick =
			stats.bytesInSendBuffer[IMMEDIATE_PRIORITY] +
			stats.bytesInSendBuffer[HIGH_PRIORITY] +
			stats.bytesInSendBuffer[MEDIUM_PRIORITY] +
			stats.bytesInSendBuffer[LOW_PRIORITY];

		if (bufferedBytesThisTick > it->second.bufferedBytesLastTick)
		{
			if (it->second.increasingBufferedBytesInARow < 0)
				it->second.increasingBufferedBytesInARow = 0;

			it->second.increasingBufferedBytesInARow++;
			if ((it->second.increasingBufferedBytesInARow > 0) &&
				(it->second.increasingBufferedBytesInARow % CONGESTION_TIME_CHECK) == 0 &&
				bufferedBytesThisTick > (1024 * 16))
			{
				if (!it->second.congestionDetected) {
					//static char message[2048];
					//RakNet::StatisticsToString(res, message, 0);
					printLog("- Client \"%s\" has a congestion. Buffered %f bytes. Ping %i.",
						remote->second.name.empty() ? it->first.ToString() : remote->second.name.c_str(),
						bufferedBytesThisTick, peer->GetAveragePing(it->first));
				}

				it->second.congestionDetected = true;
				auto it2 = remote->second.lobbies.begin();
				for (; it2 != remote->second.lobbies.end(); ++it2) {
					lobbiesCongestioned.insert(*it2);
				}
			}
		}
		else
		{
			if (it->second.increasingBufferedBytesInARow > CONGESTION_TIME_CHECK)
				it->second.increasingBufferedBytesInARow = CONGESTION_TIME_CHECK;
			it->second.increasingBufferedBytesInARow--;

			if (it->second.congestionDetected)
			{
				if (it->second.increasingBufferedBytesInARow == 0)
				{
					printLog("+ Client \"%s\" congestion relaxing. Buffered %f bytes. Ping %i.",
						remote->second.name.empty() ? it->first.ToString() : remote->second.name.c_str(),
						bufferedBytesThisTick, peer->GetAveragePing(it->first));
				}

				if (bufferedBytesThisTick == 0)
				{
					printLog("+ Client \"%s\" congestion stopped.",
						remote->second.name.empty() ? it->first.ToString() : remote->second.name.c_str());
					stopped = true;
					it->second.congestionDetected = false;
				}
			}
		}

		it->second.bufferedBytesLastTick = bufferedBytesThisTick;
	}

	if (!lobbiesCongestioned.empty())
	{
		//printLog("- CONGESTION DETECTED. Sending control message to clients.");
		RakNet::BitStream ToSend;
		ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
		ToSend.Write((brbt::packet_id_t) SERVER_CONGESTION);
		ToSend.Write((brbt::str_size_t) lobbiesCongestioned.size());
		for (auto it = lobbiesCongestioned.begin(); it != lobbiesCongestioned.end(); ++it) {
			ToSend.Write((brbt::id_t) *it);
		}

		peer->Send(&ToSend, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	}
	else if (stopped)
	{
		RakNet::BitStream ToSend;
		ToSend.Write((RakNet::MessageID) ID_SERVER_PACKET);
		ToSend.Write((brbt::packet_id_t) SERVER_CONGESTION_END);
		peer->Send(&ToSend, IMMEDIATE_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	}
}
