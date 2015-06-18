#include "ServerConnection.h"

namespace Biribit
{

Entry ServerConnectionPriv::EntryDummy;

ServerConnectionPriv::ServerConnectionPriv()
	: addr(RakNet::UNASSIGNED_SYSTEM_ADDRESS)
	, selfId(RemoteClient::UNASSIGNED_ID)
	, joinedRoom(Room::UNASSIGNED_ID)
	, joinedSlot(0)
	, joinedRoomEntries(1)
{
}

bool ServerConnectionPriv::isNull()
{
	return addr == RakNet::UNASSIGNED_SYSTEM_ADDRESS;
}

unique<Proto::RoomEntriesRequest> ServerConnectionPriv::UpdateEntries(Proto::RoomEntriesStatus* proto_entries)
{
	unique<Proto::RoomEntriesRequest> proto_entriesReq;
	if (proto_entries->has_room_id() && joinedRoom == proto_entries->room_id())
	{
		if (proto_entries->has_journal_size())
		{
			//resize journal
			std::uint32_t journal_size = proto_entries->journal_size();
			{
				std::lock_guard<std::mutex> lock(entriesMutex);
				joinedRoomEntries.resize(journal_size + 1);
			}

			//saving entries in journal
			int size = proto_entries->entries_size();
			for (int i = 0; i < size; i++)
			{
				const Proto::RoomEntry& proto_entry = proto_entries->entries(i);
				if (proto_entry.has_id() && proto_entry.has_from_slot() && proto_entry.has_entry_data())
				{
					std::uint32_t id = proto_entry.id();
					if (id > Entry::UNASSIGNED_ID && id <= journal_size)
					{
						RefSwap<Entry>& safe_entry = joinedRoomEntries[proto_entry.id()];
						Entry& entry = safe_entry.back();
						entry.id = proto_entry.id();
						entry.from_slot = proto_entry.from_slot();
						const std::string& data = proto_entry.entry_data();
						entry.data.append(data.c_str(), data.size() - 1);
						safe_entry.swap();
					}
				}
			}

			// finding out if there's more entries left to ask for
			for (std::uint32_t i = 1; i <= journal_size; i++)
			{
				if (!joinedRoomEntries[i].hasEverSwapped())
				{
					if (proto_entriesReq == nullptr)
						proto_entriesReq = unique<Proto::RoomEntriesRequest>(new Proto::RoomEntriesRequest);
					proto_entriesReq->add_entries_id(i);
				}
			}
		}
	}

	return proto_entriesReq;
}

Entry::id_t ServerConnectionPriv::GetEntriesCount()
{
	if (joinedRoom > Room::UNASSIGNED_ID)
	{
		std::lock_guard<std::mutex> lock(entriesMutex);
		return  joinedRoomEntries.size() - 1;
	}

	return Entry::UNASSIGNED_ID;
}

const Entry& ServerConnectionPriv::GetEntry(Entry::id_t id)
{
	{
		std::lock_guard<std::mutex> lock(entriesMutex);
		if (id > Entry::UNASSIGNED_ID && id < joinedRoomEntries.size())
			return joinedRoomEntries[id].front(nullptr);
	}

	return EntryDummy;
}

void ServerConnectionPriv::ResetEntries()
{
	std::lock_guard<std::mutex> lock(entriesMutex);
	joinedRoomEntries.resize(1);
}

void ServerConnectionPriv::UpdateRemoteClients()
{
	std::vector<RemoteClient>& back = clientsListReq.back();
	back.clear();

	for (auto it = clients.begin(); it != clients.end(); it++)
		if (it->id != RemoteClient::UNASSIGNED_ID)
			back.push_back(*it);

	clientsListReq.swap();
}

void ServerConnectionPriv::UpdateRooms()
{
	std::vector<Room>& back = roomsListReq.back();
	back.clear();

	for (auto it = rooms.begin(); it != rooms.end(); it++)
		if (it->id != Room::UNASSIGNED_ID)
			back.push_back(*it);

	roomsListReq.swap();
}

} // namespace Biribit