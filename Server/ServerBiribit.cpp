#include "ServerBiribit.h"
#include "PrintLog.h"

void ServerBiribit::OnDisconnectionNotification(Server& server, std::uint64_t peerId)
{
	PeerMapIter remote = GetPeer(peerId);
	if (!IsPeerValid(remote) && !remote->second.name.empty()) {
		printLog("%s (%s) has disconnected from the server.", remote->second.name.c_str(), std::to_string(peerId).c_str());
	}
	else {
		printLog("%s has disconnected from the server.", std::to_string(peerId).c_str());
	}

	Disconnected(peerId);
}

void ServerBiribit::OnNewIncomingConnection(Server& server, std::uint64_t peerId)
{
	printLog("New incoming connection from %s", std::to_string(peerId).c_str());
	Connected(peerId);
}

void ServerBiribit::OnAdvertisement(Server& server, const std::string &addr, unsigned short port, shared<Packet> packet)
{

}

void ServerBiribit::OnConnectionLost(Server& server, std::uint64_t peerId)
{
	OnDisconnectionNotification(server, peerId);
}

void ServerBiribit::OnPacket(Server& server, shared<Packet> packet)
{

}

//---------------------------------------------------------------------------//

ServerBiribit::PeerMapIter ServerBiribit::GetPeer(std::uint64_t id)
{
	return remotePeers.find(id);
}

bool ServerBiribit::IsPeerValid(PeerMapIter iter)
{
	return iter != remotePeers.end();
}

void ServerBiribit::Connected(std::uint64_t id)
{
	PeerMapIter it = GetPeer(id);
	if (!IsPeerValid(it)) {
		remotePeers.insert(PeerPair(id, Peer()));
	}
}

void ServerBiribit::Disconnected(std::uint64_t id)
{
	PeerMapIter it = remotePeers.find(id);
	if (it != remotePeers.end())
	{
		if (it->second.id != 0)
		{
			//TODO: Notify peers this disconnection
			
		}

		//Unsuscribe from all rooms
		auto it2 = it->second.rooms.begin();
		for (; it2 != it->second.rooms.end(); it2++) {
			rooms[*it2]->peers.erase(id);
		}

		remotePeers.erase(id);
	}
}
