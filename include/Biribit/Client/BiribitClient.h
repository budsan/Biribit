#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <future>

#include <Biribit/BiribitConfig.h>
#include <Biribit/Packet.h>
#include <Biribit/Client/BiribitTypes.h>
#include <Biribit/Client/BiribitEvent.h>

namespace Biribit
{
class ClientImpl;
class API_EXPORT Client
{
public:

	Client();
	virtual ~Client();

	void Connect(const char* addr = nullptr, unsigned short port = 0, const char* password = nullptr);
	void Disconnect(Connection::id_t id);
	void Disconnect();

	void DiscoverServersOnLAN(unsigned short port = 0);
	void ClearServerList();
	void RefreshServerList();

	future_vector<ServerInfo> GetServerList();
	future_vector<Connection> GetConnections();
	future_vector<RemoteClient> GetRemoteClients(Connection::id_t id);
	future_vector<Room> GetRooms(Connection::id_t id);

	/*
	const std::vector<ServerInfo>& GetServerList(std::uint32_t* revision = nullptr);
	const std::vector<Connection>& GetConnections(std::uint32_t* revision = nullptr);
	const std::vector<RemoteClient>& GetRemoteClients(Connection::id_t id, std::uint32_t* revision = nullptr);
	const std::vector<Room>& GetRooms(Connection::id_t id, std::uint32_t* revision = nullptr);
	*/

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

	void SendBroadcast(Connection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask = Packet::Unreliable);
	void SendBroadcast(Connection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask = Packet::Unreliable);

	void SendEntry(Connection::id_t id, const Packet& packet);
	void SendEntry(Connection::id_t id, const char* data, unsigned int lenght);

	std::unique_ptr<Event> PullEvent();

	Entry::id_t GetEntriesCount(Connection::id_t id);
	const Entry& GetEntry(Connection::id_t id, Entry::id_t entryId);

private:
	ClientImpl* m_impl;
};

}