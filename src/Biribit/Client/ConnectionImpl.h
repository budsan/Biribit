#pragma once

#include <vector>
#include <memory>
#include <mutex>

#include <Biribit/Packet.h>
#include <Biribit/Common/PrintLog.h>
#include <Biribit/Common/BiribitMessageIdentifiers.h>
#include <Biribit/Common/Debug.h>
#include <Biribit/Common/RefSwap.h>
#include <Biribit/Common/TaskPool.h>
#include <Biribit/Common/Types.h>
#include <Biribit/Common/Generic.h>

#include <Biribit/Client/BiribitTypes.h>

#include <RakNetTypes.h>

namespace Biribit
{

class ConnectionImpl
{
public:
	Connection data;
	RakNet::SystemAddress addr;

	ClientParameters requested;
	RemoteClient::id_t selfId;

	std::vector<RemoteClient> clients;
	RefSwap<std::vector<RemoteClient>> clientsListReq;

	std::vector<Room> rooms;
	RefSwap<std::vector<Room>> roomsListReq;

	Room::id_t joinedRoom;
	std::uint32_t joinedSlot;

	std::vector<RefSwap<Entry>> joinedRoomEntries;
	std::mutex entriesMutex;

	static Entry EntryDummy;

	ConnectionImpl();

	bool isNull();
	unique<Proto::RoomEntriesRequest> UpdateEntries(Proto::RoomEntriesStatus* proto_entries);
	Entry::id_t GetEntriesCount();
	const Entry& GetEntry(Entry::id_t id);
	void ResetEntries();

	void UpdateRemoteClients(std::vector<RemoteClient>& vect);
	void UpdateRooms(std::vector<Room>& vect);
	void UpdateRemoteClients();
	void UpdateRooms();
};

} // namespace Biribit