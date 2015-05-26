#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>

#include <Biribit/BiribitConfig.h>
#include <Biribit/Packet.h>

namespace Biribit
{

typedef std::uint32_t TimeMS;

struct API_EXPORT ServerInfo
{
	std::string name;
	std::string addr;
	std::uint32_t ping;
	unsigned short port;
	bool passwordProtected;

	ServerInfo();
};

struct API_EXPORT ServerConnection
{
	typedef std::uint32_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::string name;
	std::uint32_t ping;

	ServerConnection();
};

struct API_EXPORT RemoteClient
{
	typedef std::uint32_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::string name;
	std::string appid;

	RemoteClient();
};

struct API_EXPORT ClientParameters
{
	std::string name;
	std::string appid;

	ClientParameters();
	ClientParameters(const std::string& name, const std::string& appid);
};

struct API_EXPORT Room
{
	typedef std::uint32_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::vector<RemoteClient::id_t> slots;

	Room();
};

struct API_EXPORT Received
{
	TimeMS when;
	ServerConnection::id_t connection;
	Room::id_t room_id;
	std::uint8_t slot_id;
	Packet data;

	Received();
	Received(Received&&);
};

struct API_EXPORT Entry
{
	typedef std::uint32_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::uint32_t from_slot;
	Packet data;

	Entry();
	Entry(Entry&&);
	Entry(const Entry&) = delete;

	Entry& operator=(Entry&&);	
	const Entry& operator=(const Entry&) = delete;
};

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

	void JoinRoom(ServerConnection::id_t id, Room::id_t room_id);
	void JoinRoom(ServerConnection::id_t id, Room::id_t room_id, std::uint32_t slot_id);

	Room::id_t GetJoinedRoomId(ServerConnection::id_t id);
	std::uint32_t GetJoinedRoomSlot(ServerConnection::id_t id);

	void SendToRoom(ServerConnection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask = Packet::Unreliable);
	void SendToRoom(ServerConnection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask = Packet::Unreliable);

	std::size_t GetDataSizeOfNextReceived();
	std::unique_ptr<Received> PullReceived();

private:
	ClientImpl* m_impl;
};

}