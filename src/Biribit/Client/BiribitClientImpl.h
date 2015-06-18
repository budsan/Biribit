#pragma once

#include <Biribit/Packet.h>
#include <Biribit/Common/PrintLog.h>
#include <Biribit/Common/BiribitMessageIdentifiers.h>
#include <Biribit/Common/Debug.h>
#include <Biribit/Common/RefSwap.h>
#include <Biribit/Common/TaskPool.h>
#include <Biribit/Common/Types.h>
#include <Biribit/Common/Generic.h>

#include <Biribit/Client/BiribitTypes.h>

#include "ServerInfo.h"
#include "ServerConnection.h"

#include <map>
#include <queue>
#include <vector>
#include <array>
#include <functional>

//RakNet
#include <MessageIdentifiers.h>
#include <RakPeerInterface.h>
#include <RakNetStatistics.h>
#include <RakNetTypes.h>
#include <BitStream.h>
#include <PacketLogger.h>
#include <RakNetTypes.h>
#include <StringCompressor.h>
#include <GetTime.h>

namespace Biribit
{
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
	const std::vector<ServerInfo>& GetDiscoverInfo(std::uint32_t* revision);
	const std::vector<ServerConnection>& GetConnections(std::uint32_t* revision);
	const std::vector<RemoteClient>& GetRemoteClients(ServerConnection::id_t id, std::uint32_t* revision);

	RemoteClient::id_t GetLocalClientId(ServerConnection::id_t id);
	void SetLocalClientParameters(ServerConnection::id_t id, const ClientParameters& parameters);

	void RefreshRooms(ServerConnection::id_t id);
	const std::vector<Room>& GetRooms(ServerConnection::id_t id, std::uint32_t* revision);

	void CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots);
	void CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots, std::uint32_t slot_to_join_id);

	void JoinRandomOrCreateRoom(ServerConnection::id_t id, std::uint32_t num_slots);

	void JoinRoom(ServerConnection::id_t id, Room::id_t room_id);
	void JoinRoom(ServerConnection::id_t id, Room::id_t room_id, std::uint32_t slot_id);

	Room::id_t GetJoinedRoomId(ServerConnection::id_t id);
	std::uint32_t GetJoinedRoomSlot(ServerConnection::id_t id);

	void SendBroadcast(ServerConnection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask);
	void SendBroadcast(ServerConnection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask);

	void SendEntry(ServerConnection::id_t id, const Packet& packet);
	void SendEntry(ServerConnection::id_t id, const char* data, unsigned int lenght);

	std::size_t GetDataSizeOfNextReceived();
	std::unique_ptr<Received> PullReceived();

	Entry::id_t GetEntriesCount(ServerConnection::id_t id);
	const Entry& GetEntry(ServerConnection::id_t id, Entry::id_t entryId);

private:
	RakNet::RakPeerInterface *m_peer;
	unique<TaskPool> m_pool;

	void SendBroadcast(ServerConnection::id_t id, shared<Packet> packet, Packet::ReliabilityBitmask mask);
	void SendEntry(ServerConnection::id_t id, shared<Packet> packet);

	void SendProtocolMessageID(RakNet::MessageID msg, const RakNet::AddressOrGUID systemIdentifier);
	bool WriteMessage(RakNet::BitStream& bstream, RakNet::MessageID msgId, const ::google::protobuf::MessageLite& msg);
	template<typename T> bool ReadMessage(T& msg, RakNet::BitStream& bstream);
	template<typename T> bool ReadMessage(T& msg, Packet& packet);

	static void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
	void RakNetUpdated();
	void HandlePacket(RakNet::Packet*);

	void ConnectedAt(RakNet::SystemAddress);
	void DisconnectFrom(RakNet::SystemAddress);

	enum TypeUpdateRemoteClient { UPDATE_REMOTE_CLIENT, UPDATE_SELF_CLIENT, UPDATE_REMOTE_DISCONNECTION };
	void UpdateRemoteClient(RakNet::SystemAddress addr, const Proto::Client* proto_client, TypeUpdateRemoteClient type);
	void UpdateRoom(RakNet::SystemAddress addr, const Proto::Room* proto_room);

	static void PopulateServerInfo(ServerInfoPriv&, const Proto::ServerInfo*);
	static void PopulateRemoteClient(RemoteClient&, const Proto::Client*);
	static void PopulateRoom(Room&, const Proto::Room*);

	void UpdateDiscoverInfo();
	void UpdateConnections();

	Generic::TempBuffer m_buffer;

	std::map<RakNet::SystemAddress, ServerInfoPriv> serverList;
	RefSwap<std::vector<ServerInfo>> serverListReq;

	std::array<ServerConnectionPriv, CLIENT_MAX_CONNECTIONS + 1> m_connections;
	RefSwap<std::vector<ServerConnection>> connectionsListReq;
	RakNet::TimeMS m_current;
	RakNet::TimeMS m_lastDirtyDueTime;

	std::queue<std::unique_ptr<Received>> m_receivedPending;
	std::mutex m_receivedMutex;
};

} // namespace Biribit