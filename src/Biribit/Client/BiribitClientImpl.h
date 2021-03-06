#pragma once

#include <Biribit/Packet.h>
#include <Biribit/Common/PrintLog.h>
#include <Biribit/Common/BiribitMessageIdentifiers.h>
#include <Biribit/Common/Debug.h>
#include <Biribit/Common/TaskPool.h>
#include <Biribit/Common/Types.h>
#include <Biribit/Common/Generic.h>

#include <Biribit/Client/BiribitTypes.h>
#include <Biribit/Client/BiribitEvent.h>

#include "ServerInfoImpl.h"
#include "ConnectionImpl.h"

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
	void Disconnect(Connection::id_t id);
	void Disconnect();

	void DiscoverServersOnLAN(unsigned short port);
	void ClearServerList();
	void RefreshServerList();

	future_vector<ServerInfo> GetServerList();
	future_vector<Connection> GetConnections();
	future_vector<RemoteClient> GetRemoteClients(Connection::id_t id);
	future_vector<Room> GetRooms(Connection::id_t id);

	RemoteClient::id_t GetLocalClientId(Connection::id_t id);
	void SetLocalClientParameters(Connection::id_t id, const ClientParameters& parameters);

	void RefreshRooms(Connection::id_t id);

	void CreateRoom(Connection::id_t id, Room::slot_id_t num_slots);
	void CreateRoom(Connection::id_t id, Room::slot_id_t num_slots, Room::slot_id_t slot_to_join_id);

	void JoinRandomOrCreateRoom(Connection::id_t id, Room::slot_id_t num_slots);

	void JoinRoom(Connection::id_t id, Room::id_t room_id);
	void JoinRoom(Connection::id_t id, Room::id_t room_id, Room::slot_id_t slot_id);

	Room::id_t GetJoinedRoomId(Connection::id_t id);
	Room::slot_id_t GetJoinedRoomSlot(Connection::id_t id);

	void SendBroadcast(Connection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask);
	void SendBroadcast(Connection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask);

	void SendEntry(Connection::id_t id, const Packet& packet);
	void SendEntry(Connection::id_t id, const char* data, unsigned int lenght);

	template<class T> void PushEvent(std::unique_ptr<T> to_push)
	{
		std::lock_guard<std::mutex> lock(m_eventMutex);
		m_eventQueue.push(std::move(to_push));
	}

	std::unique_ptr<Event> PullEvent();

	Entry::id_t GetEntriesCount(Connection::id_t id);
	const Entry& GetEntry(Connection::id_t id, Entry::id_t entryId);

private:
	RakNet::RakPeerInterface *m_peer;
	unique<TaskPool> m_pool;

	void SendBroadcast(Connection::id_t id, shared<Packet> packet, Packet::ReliabilityBitmask mask);
	void SendEntry(Connection::id_t id, shared<Packet> packet);

	void SendProtocolMessageID(RakNet::MessageID msg, const RakNet::AddressOrGUID systemIdentifier);
	bool WriteMessage(RakNet::BitStream& bstream, RakNet::MessageID msgId, const ::google::protobuf::MessageLite& msg);
	template<typename T> bool ReadMessage(T& msg, RakNet::BitStream& bstream);
	template<typename T> bool ReadMessage(T& msg, Packet& packet);

	static void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
	void RakNetUpdated();
	void HandlePacket(RakNet::Packet*);

	void ConnectedAt(RakNet::SystemAddress);
	void DisconnectFrom(RakNet::SystemAddress);

	enum TypeUpdateRemoteClient { UPDATE_CLIENT, UPDATE_DISCONNECTION };
	void UpdateRemoteClient(RakNet::SystemAddress addr, const Proto::Client* proto_client, TypeUpdateRemoteClient type);
	void UpdateRoom(RakNet::SystemAddress addr, const Proto::Room* proto_room);

	static void PopulateServerInfo(ServerInfoImpl&, const Proto::ServerInfo*);
	static void PopulateRemoteClient(RemoteClient&, const Proto::Client*);
	static void PopulateRoom(Room&, const Proto::Room*);

	void UpdateServerList(std::vector<ServerInfo>& vect);
	void UpdateConnections(std::vector<Connection>& vect);
	void PushServerListEvent();
	void PushConnectionsEvent(Connection::id_t id, ConnectionEvent::EventType type);

	Generic::TempBuffer m_buffer;

	std::map<RakNet::SystemAddress, ServerInfoImpl> serverList;
	std::array<ConnectionImpl, CLIENT_MAX_CONNECTIONS + 1> m_connections;

	std::queue<std::unique_ptr<Event>> m_eventQueue;
	std::mutex m_eventMutex;
};

} // namespace Biribit