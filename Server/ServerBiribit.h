#pragma once

#include <set>
#include <map>

#include "ServerListener.h"

class ServerBiribit : public ServerListener
{
protected:
	struct Peer
	{
		brbt::id_t id;
		std::string name;
		std::set<brbt::id_t> rooms;

		double        bufferedBytesLastTick;
		unsigned char increasingBufferedBytesInARow;
		bool          congestionDetected;

		Peer() :
			name(),
			id(0),
			bufferedBytesLastTick(0),
			increasingBufferedBytesInARow(0),
			congestionDetected(false) {}
	};

	typedef std::map<std::uint64_t, Peer>  PeerMap;
	typedef std::pair<std::uint64_t, Peer> PeerPair;
	typedef PeerMap::iterator PeerMapIter;

	PeerMap remotePeers;
	std::map<std::string, PeerMapIter> peerNames;
	std::map<brbt::id_t, PeerMapIter> peerIDs;
	brbt::id_t  peerLastUniqueID;

	struct Room
	{
		brbt::id_t id;
		std::string name;
		std::set<std::uint64_t> peers;

		Room() :
			id(0) {}

		Room(brbt::id_t id, const std::string& name) :
			id(id), name(name) {}

	private:
		Room(const Room&);
		Room& operator=(const Room&);
	};

	std::vector<std::unique_ptr<Room>> rooms;
	std::map<std::string, brbt::id_t> roomsNames;

public:

	void OnDisconnectionNotification(Server& server, std::uint64_t id) override;
	void OnNewIncomingConnection(Server& server, std::uint64_t id) override;
	void OnAdvertisement(Server& server, const std::string &addr, unsigned short port, shared<Packet> packet) override;
	void OnConnectionLost(Server& server, std::uint64_t id) override;
	void OnPacket(Server& server, shared<Packet> packet) override;

protected:

	PeerMapIter GetPeer(std::uint64_t id);
	bool IsPeerValid(PeerMapIter iter);

	void Connected(std::uint64_t id);
	void Disconnected(std::uint64_t id);
};
