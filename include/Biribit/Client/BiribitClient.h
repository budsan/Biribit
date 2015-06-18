#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>

#include <Biribit/BiribitConfig.h>
#include <Biribit/Packet.h>
#include <Biribit/Client/BiribitTypes.h>

namespace Biribit
{
class ClientImpl;
class API_EXPORT Client
{
public:

	Client();
	virtual ~Client();

	void Connect(const char* addr = nullptr, unsigned short port = 0, const char* password = nullptr);
	void Disconnect(ServerConnection::id_t id);
	void Disconnect();

	void DiscoverOnLan(unsigned short port = 0);
	void ClearDiscoverInfo();
	void RefreshDiscoverInfo();

	const std::vector<ServerInfo>& GetDiscoverInfo(std::uint32_t* revision = nullptr);
	const std::vector<ServerConnection>& GetConnections(std::uint32_t* revision = nullptr);
	const std::vector<RemoteClient>& GetRemoteClients(ServerConnection::id_t id, std::uint32_t* revision = nullptr);

	RemoteClient::id_t GetLocalClientId(ServerConnection::id_t id);
	void SetLocalClientParameters(ServerConnection::id_t id, const ClientParameters& parameters);

	void RefreshRooms(ServerConnection::id_t id);
	const std::vector<Room>& GetRooms(ServerConnection::id_t id, std::uint32_t* revision = nullptr);

	void CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots);
	void CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots, std::uint32_t slot_to_join_id);

	void JoinRandomOrCreateRoom(ServerConnection::id_t id, std::uint32_t num_slots);

	void JoinRoom(ServerConnection::id_t id, Room::id_t room_id);
	void JoinRoom(ServerConnection::id_t id, Room::id_t room_id, std::uint32_t slot_id);

	Room::id_t GetJoinedRoomId(ServerConnection::id_t id);
	std::uint32_t GetJoinedRoomSlot(ServerConnection::id_t id);

	void SendBroadcast(ServerConnection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask = Packet::Unreliable);
	void SendBroadcast(ServerConnection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask = Packet::Unreliable);

	void SendEntry(ServerConnection::id_t id, const Packet& packet);
	void SendEntry(ServerConnection::id_t id, const char* data, unsigned int lenght);

	std::size_t GetDataSizeOfNextReceived();
	std::unique_ptr<Received> PullReceived();

	Entry::id_t GetEntriesCount(ServerConnection::id_t id);
	const Entry& GetEntry(ServerConnection::id_t id, Entry::id_t entryId);

private:
	ClientImpl* m_impl;
};

}