#pragma once

#include <map>
#include <set>
#include <vector>
#include <string>
#include <memory>

#include <mutex>
#include <condition_variable>

#include <chrono>

#include "RakPeerInterface.h"
#include "Generic.h"

#include "Types.h"
#include "SignalTypes.h"

struct Server
{
public:

	 Server();
	~Server();

	bool bInit(const char* password, int port = 0, const char* name = NULL);

	void ChangeName(const char* name = NULL);

	void Run();
	void Close();

	// Call this from Raknet update thread
	void WakeLoop(RakNet::RakPeerInterface *_peer);

private:

	struct Peer
	{
		std::string    name;
		std::set<brbt::id_t> lobbies;
		brbt::id_t           id;

		double        bufferedBytesLastTick;
		unsigned char increasingBufferedBytesInARow;
		bool          congestionDetected;

		Peer() : 
			name(), 
			id (0),
			bufferedBytesLastTick(0), 
			increasingBufferedBytesInARow(0), 
			congestionDetected(false) {}
	};

	struct Lobby
	{
		brbt::id_t                       id;
		std::string                     name;
		std::set<RakNet::SystemAddress> peers;

		Lobby() : 
			id (0) {}

		Lobby(brbt::id_t id, const std::string& name) : 
			id (id), name(name) {}

	private:
		Lobby(const Lobby&);
		Lobby& operator=(const Lobby&);
	};

	typedef std::map<RakNet::SystemAddress, Peer>  PeerMap;
	typedef std::pair<RakNet::SystemAddress, Peer> PeerPair;
	typedef PeerMap::iterator                      PeerMapIter;
	typedef unique<Lobby> LobbyPtr;

	static PacketReliability ParseReliability(PacketReliabilityBitmask mask);

	void Destroy();

	brbt::id_t bSetPeerName(RakNet::SystemAddress* address, const std::string &name);
	PeerMapIter pGetPeer(const RakNet::SystemAddress* address);
	bool IsPeerValid(PeerMapIter iter);

	void Connected(RakNet::SystemAddress* address);
	void Disconnected(RakNet::SystemAddress* address);

	brbt::id_t SubscribeLobby(const std::string &name, PeerMapIter remote);
	bool UnsubscribeLobby(brbt::id_t id, PeerMapIter remote);

	void HandlePacket(RakNet::Packet* p);
	void HandleLobbyPacket(RakNet::Packet* p, RakNet::BitStream *stream, PeerMapIter remote, PacketReliability reliability = UNRELIABLE);
	void HandleServerPacket(RakNet::Packet* p, RakNet::BitStream *stream, PeerMapIter remote);
	void CheckCongestion();

	std::string serverName;

	PeerMap remotePeers;
	std::map<std::string, PeerMapIter> peerNames;
	std::map<brbt::id_t, PeerMapIter> peerIDs;
	brbt::id_t  peerLastUniqueID;

	std::vector<std::unique_ptr<Lobby>> lobbies;
	std::map<std::string, brbt::id_t> lobbiesNames;
	
	RakNet::RakPeerInterface *peer;
	Generic::TempBuffer	tempBuffer;

	std::mutex loopMutex;
	std::condition_variable loopCondVar;

	typedef std::chrono::time_point<std::chrono::system_clock> TimePoint;
	TimePoint timer;
	double elapsedTime;
	bool forceExit;
};
