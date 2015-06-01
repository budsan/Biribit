#pragma once

#include <Biribit/Common/TaskPool.h>
#include <Biribit/Common/Types.h>
#include <Biribit/Common/Generic.h>
#include <Biribit/Common/BiribitMessageIdentifiers.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <cstdint>

//RakNet
#include <RakPeerInterface.h>

class RakNetServer
{
	RakNet::RakPeerInterface *m_peer;
	std::string m_name;
	bool m_passwordProtected;

	struct Client
	{
		typedef std::uint32_t id_t;
		enum { UNASSIGNED_ID = 0 };

		id_t id;
		std::string name;
		std::string appid;
		std::uint32_t joined_room;
		std::uint32_t joined_slot;
		RakNet::SystemAddress addr;

		Client();
	};

	std::vector<unique<Client>> m_clients;
	std::map<RakNet::SystemAddress, Client::id_t> m_clientAddrMap;
	std::map<std::string, Client::id_t> m_clientNameMap;

	struct Room
	{
		typedef std::uint32_t id_t;
		enum { UNASSIGNED_ID = 0 };

		id_t id;
		std::uint32_t joined_clients_count;
		std::vector<Client::id_t> slots;
		std::string appid;

		struct Entry
		{
			typedef std::uint32_t id_t;
			enum { UNASSIGNED_ID = 0 };

			std::uint32_t from_slot;
			std::string data;

			Entry();
		};

		std::vector<Entry> journal;

		Room();
	};

	std::vector<unique<Room>> m_rooms;
	std::map<std::string, std::set<Room::id_t>> m_roomAppIdMap;

	unique<Client>& GetClient(RakNet::SystemAddress addr);

	Client::id_t NewClient(RakNet::SystemAddress addr);
	void RemoveClient(RakNet::SystemAddress addr);
	void UpdateClient(RakNet::SystemAddress addr, Proto::ClientUpdate* proto_update);

	void ListRooms(RakNet::SystemAddress addr);
	void JoinRandomOrCreate(RakNet::SystemAddress addr, Proto::RoomCreate* proto_create);
	void CreateRoom(RakNet::SystemAddress addr, Proto::RoomCreate* proto_create);
	void JoinRoom(RakNet::SystemAddress addr, Proto::RoomJoin* proto_join);
	bool LeaveRoom(unique<Client>& client);
	void RoomChanged(unique<Room>& room, RakNet::SystemAddress extra_addr_to_notify = RakNet::UNASSIGNED_SYSTEM_ADDRESS);

	void SendRoomBroadcast(RakNet::SystemAddress addr, RakNet::Time timeStamp, RakNet::BitStream& in);
	void SendRoomEntry(RakNet::SystemAddress addr, RakNet::BitStream& in);
	void RoomEntriesRequest(RakNet::SystemAddress addr, Proto::RoomEntriesRequest* proto_entriesReq);

	void PopulateProtoServerInfo(Proto::ServerInfo* proto_info);
	void PopulateProtoClient(unique<Client>& client, Proto::Client* proto_client);
	void PopulateProtoRoom(unique<Room>& room, Proto::Room* proto_room);
	void PopulateProtoRoomJoin(unique<Client>& client, Proto::RoomJoin* proto_join);
	void PopulateProtoRoomEntriesStatus(unique<Room>& room, Proto::RoomEntriesStatus* proto_entries);

	unique<TaskPool> m_pool;
	Generic::TempBuffer m_buffer;

	static void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
	void RakNetUpdated();
	void HandlePacket(RakNet::Packet*);

	bool WriteMessage(RakNet::BitStream& bstream, RakNet::MessageID msgId, ::google::protobuf::MessageLite& msg);
	template<typename T> bool ReadMessage(T& msg, RakNet::BitStream& bstream);

	void SendErrorCode(std::uint32_t error_code, RakNet::AddressOrGUID systemIdentifier);

public:

	RakNetServer();

	bool Run(unsigned short port = 0, const char* name = NULL, const char* password = NULL);
	bool isRunning();
	bool Close();
};
