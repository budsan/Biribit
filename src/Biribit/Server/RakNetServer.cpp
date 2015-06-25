#include <Biribit/Server/RakNetServer.h>
#include <Biribit/Common/Types.h>
#include <Biribit/Common/PrintLog.h>
#include <Biribit/Common/Debug.h>
#include <Biribit/Common/Generic.h>
#include <Biribit/Common/BiribitMessageIdentifiers.h>

#include <sstream>
#include <chrono>

//RakNet
#include <MessageIdentifiers.h>
#include <RakNetStatistics.h>
#include <RakNetTypes.h>
#include <BitStream.h>
#include <RakSleep.h>
#include <PacketLogger.h>

RakNetServer::Client::Client()
	: id(Client::UNASSIGNED_ID)
	, name()
	, appid()
	, joined_room(Room::UNASSIGNED_ID)
	, joined_slot(0)
	, addr(RakNet::UNASSIGNED_SYSTEM_ADDRESS)
{
}

RakNetServer::Room::Room()
	: id(Room::UNASSIGNED_ID)
	, joined_clients_count(0)
	, journal(1)
{
}

RakNetServer::Room::Entry::Entry()
	: from_slot(0)
{
}

const char* randomNames[] = {
	"Arianne", "Kesha", "Minerva",
	"Dianna", "Daisey", "Edna",
	"Katerine", "Delpha", "Lucinda",
	"Tierra", "Tyisha", "Zana",
	"Tiffani", "Dorris", "Venice",
	"June", "Lakeisha", "Mafalda",
	"Ardella", "Chantell", "Zaida",
	"Lyn", "Rachael", "Sulema",
	"Raymonde", "Arline", "Martine",
	"Marylyn", "Maribeth", "Genevieve",
	"Lucila", "Sofia", "Madeline",
	"Berneice", "Mozella", "Angelique",
	"Amelia", "Marylynn", "Felice",
	"Tessa", "Sharapova", "Blossom",
	"Keren", "Susan", "Ada",
	"Shenika", "Tandy", "Rebecca",
	"Farah", "Latrisha"
};

template<int N> int sizeof_string_array(const char* (&s)[N]) { return N; }

RakNetServer::RakNetServer()
	: m_peer(nullptr)
	, m_clients(1)
	, m_rooms(1)
{
}

unique<RakNetServer::Client>& RakNetServer::GetClient(RakNet::SystemAddress addr)
{
	auto it = m_clientAddrMap.find(addr);
	BIRIBIT_ASSERT(it != m_clientAddrMap.end());
	BIRIBIT_ASSERT(it->second < m_clients.size());
	BIRIBIT_ASSERT(m_clients[it->second] != nullptr);
	return m_clients[it->second];
}

RakNetServer::Client::id_t RakNetServer::NewClient(RakNet::SystemAddress addr)
{
	BIRIBIT_ASSERT(m_clientAddrMap.find(addr) == m_clientAddrMap.end());

	Client::id_t i = 1;
	for (; i < m_clients.size() && m_clients[i] != nullptr; i++);

	if (i < m_clients.size())
		m_clients[i] = unique<Client>(new Client());
	else
		m_clients.push_back(unique<Client>(new Client()));

	m_clients[i]->id = i;
	m_clients[i]->appid.clear();
	m_clients[i]->addr = addr;
	m_clientAddrMap[addr] = i;

	int perm_name = 1;
	while (perm_name != 0)
	{
		using clock = std::chrono::system_clock;
		auto timer = clock::now();
		auto now_c = clock::to_time_t(timer) + perm_name;

		int nameID = now_c % sizeof_string_array(randomNames);
		std::string name = randomNames[nameID] + std::to_string(std::hash<int>()(i+nameID)& 0x7F);

		auto itName = m_clientNameMap.find(name);
		if (itName == m_clientNameMap.end())
		{
			m_clientNameMap[name] = i;
			m_clients[i]->name = name;
			perm_name = 0;
		}
		else
			perm_name++;
	}

	Proto::Client proto_client;
	PopulateProtoClient(m_clients[i], &proto_client);
	{
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_CLIENT_STATUS_UPDATED, proto_client))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	}
	{
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_CLIENT_SELF_STATUS, proto_client))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, addr, false);
	}

	return i;
}

void RakNetServer::RemoveClient(RakNet::SystemAddress addr)
{
	auto it = m_clientAddrMap.find(addr);
	BIRIBIT_ASSERT(it != m_clientAddrMap.end());
	BIRIBIT_ASSERT(it->second < m_clients.size());
	BIRIBIT_ASSERT(m_clients[it->second] != nullptr);
	unique<Client>& client = m_clients[it->second];

	Proto::RoomJoin proto_join;
	proto_join.set_id(0);
	JoinRoom(addr, &proto_join);

	Proto::Client proto_client;
	PopulateProtoClient(client, &proto_client);

	if (!client->name.empty())
	{
		std::size_t erased_count = m_clientNameMap.erase(client->name);
		BIRIBIT_ASSERT(erased_count > 0);
	}
	
	m_clients[it->second] = nullptr;
	m_clientAddrMap.erase(it);

	RakNet::BitStream bstream;
	if (WriteMessage(bstream, ID_CLIENT_DISCONNECTED, proto_client))
		m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void RakNetServer::UpdateClient(RakNet::SystemAddress addr, Proto::ClientUpdate* proto_update)
{
	unique<Client>& client = GetClient(addr);
	bool updated = false;
	if (proto_update->has_name())
	{
		const std::string& name = proto_update->name();
		bool already_used = false, success = false;
		std::uint32_t tries = 0;
		while (!success && tries < 100)
		{
			std::string current_name = tries > 0 ? name + std::to_string(tries) : name;
			auto itName = m_clientNameMap.find(current_name);
			if (itName != m_clientNameMap.end())
			{
				if (client->id != itName->second)
					already_used = true;
			}
			else
			{
				printLog("Client(%d) \"%s\" changed name to \"%s\".", client->id, client->name.c_str(), current_name.c_str());
				m_clientNameMap[current_name] = client->id;
				client->name = current_name;
				updated = true;
				success = true;
			}

			++tries;
		}

		// another client is using that name
		if (already_used)
			SendErrorCode(WARN_CLIENT_NAME_IN_USE, addr);
	}

	if (proto_update->has_appid() && client->appid != proto_update->appid())
	{
		client->appid = proto_update->appid();
		printLog("Client(%d) \"%s\" changed appid to \"%s\".", client->id, client->name.c_str(), proto_update->appid().c_str());
		updated = true;

		LeaveRoom(client);
	}

	if (updated)
	{
		Proto::Client proto_client;
		PopulateProtoClient(client, &proto_client);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_CLIENT_STATUS_UPDATED, proto_client))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	}
}

void RakNetServer::ListRooms(RakNet::SystemAddress addr)
{
	unique<Client>& client = GetClient(addr);
	if (client->appid.empty()) {
		SendErrorCode(WARN_CANNOT_LIST_ROOMS_WITHOUT_APPID, addr);
		printLog("WARN: Client (%d) \"%s\" can't list rooms without appid.", client->id, client->name.c_str());
		return;
	}

	Proto::RoomList proto_list;
	auto set_it = m_roomAppIdMap.find(client->appid);
	if (set_it != m_roomAppIdMap.end())
	{
		for (auto it = set_it->second.begin(); it != set_it->second.end(); it++) {
			BIRIBIT_ASSERT(m_rooms[*it] != nullptr);
			Proto::Room* room_to_add = proto_list.add_rooms();
			PopulateProtoRoom(m_rooms[*it], room_to_add);
		}
	}

	RakNet::BitStream bstream;
	if (WriteMessage(bstream, ID_ROOM_LIST_RESPONSE, proto_list)) {
		m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, addr, false);
	}
}

void RakNetServer::JoinRandomOrCreate(RakNet::SystemAddress addr, Proto::RoomCreate* proto_create)
{
	unique<Client>& client = GetClient(addr);
	if (client->appid.empty()) {
		SendErrorCode(WARN_CANNOT_LIST_ROOMS_WITHOUT_APPID, addr);
		printLog("WARN: Client (%d) \"%s\" can't list rooms without appid.", client->id, client->name.c_str());
		return;
	}

	auto set_it = m_roomAppIdMap.find(client->appid);
	if (set_it != m_roomAppIdMap.end())
	{
		for (auto it = set_it->second.begin(); it != set_it->second.end(); it++) {
			BIRIBIT_ASSERT(m_rooms[*it] != nullptr);
			unique<Room>& room = m_rooms[*it];
			if (room->joined_clients_count < room->slots.size())
			{
				Proto::RoomJoin proto_join;
				proto_join.set_id(room->id);
				JoinRoom(addr, &proto_join);
				return;
			}
		}
	}

	CreateRoom(addr, proto_create);
}

void RakNetServer::CreateRoom(RakNet::SystemAddress addr, Proto::RoomCreate* proto_create)
{
	unique<Client>& client = GetClient(addr);
	if (client->appid.empty()) {
		SendErrorCode(WARN_CANNOT_CREATE_ROOM_WITHOUT_APPID, addr);
		printLog("WARN: Client (%d) \"%s\" can't create a room without appid.", client->id, client->name.c_str());
		return;
	}

	if (!proto_create->has_client_slots() || proto_create->client_slots() == 0) {
		SendErrorCode(WARN_CANNOT_CREATE_ROOM_WITH_WRONG_SLOT_NUMBER, addr);
		printLog("WARN: Client (%d) \"%s\" tried to create a room with a wrong slot number.", client->id, client->name.c_str());
		return;
	}

	if (proto_create->client_slots() > 0xFF) {
		SendErrorCode(WARN_CANNOT_CREATE_ROOM_WITH_TOO_MANY_SLOTS, addr);
		printLog("WARN: Client (%d) \"%s\" tried to create a room with too many slots.", client->id, client->name.c_str());
		return;
	}

	std::size_t i = 1;
	for (; i < m_rooms.size() && m_rooms[i] != nullptr; i++);
	if (i == m_rooms.size())
		m_rooms.push_back(nullptr);

	m_rooms[i] = unique<Room>(new Room());
	unique<Room>& room = m_rooms[i];
	room->id = i;
	room->appid = client->appid;
	room->slots.resize(proto_create->client_slots(), Client::UNASSIGNED_ID);

	std::set<Room::id_t>& room_set = m_roomAppIdMap[room->appid];
	auto result = room_set.insert(room->id);
	BIRIBIT_ASSERT(result.second);

	printLog("Created room %d for the app %s.", room->id, room->appid.c_str());
		
	Proto::RoomJoin proto_join;
	proto_join.set_id(room->id);
	if (proto_create->has_slot_to_join())
		proto_join.set_slot_to_join(proto_create->slot_to_join());

	JoinRoom(addr, &proto_join);
}

void RakNetServer::JoinRoom(RakNet::SystemAddress addr, Proto::RoomJoin* proto_join)
{
	unique<Client>& client = GetClient(addr);
	if (!proto_join->has_id()) {
		SendErrorCode(WARN_CANNOT_JOIN_WITHOUT_ROOM_ID, addr);
		printLog("WARN: Client (%d) \"%s\" sent RoomJoin without room id.", client->id, client->name.c_str());
		return;
	}
	
	bool something_changed = false;
	std::uint32_t id = proto_join->id();
	if (id == Room::UNASSIGNED_ID)
	{
		//Client just want to leave the room
		something_changed = LeaveRoom(client);
		{
			Proto::RoomJoin proto_join;
			PopulateProtoRoomJoin(client, &proto_join);
			RakNet::BitStream bstream;
			if (WriteMessage(bstream, ID_ROOM_JOIN_RESPONSE, proto_join))
				m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, addr, false);
		}
	}
	else
	{
		if (id != Room::UNASSIGNED_ID)
		{
			if (m_rooms[id] == nullptr) {
				SendErrorCode(WARN_CANNOT_JOIN_TO_UNEXISTING_ROOM, addr);
				printLog("WARN: Client (%d) \"%s\" tried to join to unexisting room.", client->id, client->name.c_str());
				return;
			}

			if (m_rooms[id]->appid != client->appid) {
				SendErrorCode(WARN_CANNOT_JOIN_TO_OTHER_APP_ROOM, addr);
				printLog("WARN: Client (%d) \"%s\" tried to join other app's room.", client->id, client->name.c_str());
				return;
			}
		}

		//Leaving old room, joining new one
		if (client->joined_room != id)
		{
			std::uint32_t slot;
			unique<Room>& room = m_rooms[id];
			if (proto_join->has_slot_to_join())
			{
				slot = proto_join->slot_to_join();
				if (slot >= room->slots.size() || room->slots[slot] != Client::UNASSIGNED_ID) {
					if (slot >= room->slots.size())
						SendErrorCode(WARN_CANNOT_JOIN_TO_INVALID_SLOT, addr);
					else
						SendErrorCode(WARN_CANNOT_JOIN_TO_OCCUPIED_SLOT, addr);
					printLog("WARN: Client (%d) \"%s\" tried to join an invalid slot.", client->id, client->name.c_str());
					return;
				}
			}
			else
			{
				for (slot = 0; slot < room->slots.size() && room->slots[slot] != Client::UNASSIGNED_ID; slot++);
				if (slot >= m_rooms[id]->slots.size()) {
					SendErrorCode(WARN_CANNOT_JOIN_TO_FULL_ROOM, addr);
					printLog("WARN: Client (%d) \"%s\" tried to join a full room.", client->id, client->name.c_str());
					return;
				}
			}
			
			LeaveRoom(client);
			room->slots[slot] = client->id;
			room->joined_clients_count++;
			client->joined_room = id;
			client->joined_slot = slot;
			printLog("Client (%d) \"%s\" joins room %d.", client->id, client->name.c_str(), room->id);
			RoomChanged(room);

			{
				Proto::RoomJoin proto_join;
				PopulateProtoRoomJoin(client, &proto_join);
				RakNet::BitStream bstream;
				if (WriteMessage(bstream, ID_ROOM_JOIN_RESPONSE, proto_join))
					m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, addr, false);
			}
			{
				Proto::RoomEntriesStatus proto_entries;
				PopulateProtoRoomEntriesStatus(room, &proto_entries);
				RakNet::BitStream bstream;
				if (WriteMessage(bstream, ID_JOURNAL_ENTRIES_STATUS, proto_entries))
					m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, addr, false);
			}
			
		}
		else if (proto_join->has_slot_to_join() && client->joined_slot != proto_join->slot_to_join())
		{
			//Slot swapping
			unique<Room>& room = m_rooms[id];
			std::uint32_t oldslot = client->joined_slot;
			std::uint32_t slot = proto_join->slot_to_join();
			if (slot >= room->slots.size() || room->slots[slot] != Client::UNASSIGNED_ID) {
				if (slot >= room->slots.size())
					SendErrorCode(WARN_CANNOT_JOIN_TO_INVALID_SLOT, addr);
				else
					SendErrorCode(WARN_CANNOT_JOIN_TO_OCCUPIED_SLOT, addr);
				printLog("WARN: Client (%d) \"%s\" tried to join an invalid slot.", client->id, client->name.c_str());
				return;
			}
	
			room->slots[oldslot] = Client::UNASSIGNED_ID;
			room->slots[slot] = client->id;
			client->joined_slot = slot;

			printLog("Client (%d) \"%s\" swaps slot from %d to %d in room %d.", client->id, client->name.c_str(), oldslot, slot, room->id);
			RoomChanged(room);

			{
				Proto::RoomJoin proto_join;
				PopulateProtoRoomJoin(client, &proto_join);
				RakNet::BitStream bstream;
				if (WriteMessage(bstream, ID_ROOM_JOIN_RESPONSE, proto_join))
					m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, addr, false);
			}
			{
				Proto::RoomEntriesStatus proto_entries;
				PopulateProtoRoomEntriesStatus(room, &proto_entries);
				RakNet::BitStream bstream;
				if (WriteMessage(bstream, ID_JOURNAL_ENTRIES_STATUS, proto_entries))
					m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, addr, false);
			}
		}
	}
}

bool RakNetServer::LeaveRoom(unique<Client>& client)
{
	if (client->joined_room > 0)
	{
		BIRIBIT_ASSERT(m_rooms[client->joined_room] != nullptr);
		BIRIBIT_ASSERT(m_rooms[client->joined_room]->slots[client->joined_slot] == client->id);

		unique<Room>& room = m_rooms[client->joined_room];
		room->slots[client->joined_slot] = Client::UNASSIGNED_ID;
		room->joined_clients_count--;
		client->joined_room = Room::UNASSIGNED_ID;
		client->joined_slot = 0;

		printLog("Client (%d) \"%s\" leaves room %d.", client->id, client->name.c_str(), room->id);
		
		if (room->joined_clients_count == 0) {
			std::size_t erased = m_roomAppIdMap[room->appid].erase(room->id);
			BIRIBIT_ASSERT(erased > 0);
			if (m_roomAppIdMap[room->appid].empty())
				m_roomAppIdMap.erase(room->appid);
	
			printLog("Room %d is empty. Closing room.", room->id);
			m_rooms[room->id] = nullptr;
		}
		else
			RoomChanged(room, client->addr);

		return true;
	}

	return false;
}
void RakNetServer::RoomChanged(unique<Room>& room, RakNet::SystemAddress extra_addr_to_notify)
{
	Proto::Room proto_room;
	PopulateProtoRoom(room, &proto_room);
	RakNet::BitStream bstream;
	if (WriteMessage(bstream, ID_ROOM_STATUS, proto_room)) {
		for (auto it = room->slots.begin(); it != room->slots.end(); it++) {
			if (*it != Client::UNASSIGNED_ID) {
				BIRIBIT_ASSERT(m_clients[*it] != nullptr);
				m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, m_clients[*it]->addr, false);
			}
		}

		if (extra_addr_to_notify != RakNet::UNASSIGNED_SYSTEM_ADDRESS)
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, extra_addr_to_notify, false);
	}
}

void RakNetServer::SendRoomBroadcast(RakNet::SystemAddress addr, RakNet::Time timeStamp, RakNet::BitStream& in)
{
	unique<Client>& client = GetClient(addr);
	if (client->joined_room > 0)
	{
		BIRIBIT_ASSERT(m_rooms[client->joined_room] != nullptr);
		BIRIBIT_ASSERT(m_rooms[client->joined_room]->slots[client->joined_slot] == client->id);
		unique<Room>& room = m_rooms[client->joined_room];


		std::uint8_t uint8_reliability;
		in.Read(uint8_reliability);
		PacketReliability reliability = (PacketReliability) uint8_reliability;
		switch (reliability)
		{
		case UNRELIABLE:
		case RELIABLE:
		case RELIABLE_ORDERED:
			break;
		default:
			reliability = UNRELIABLE;
			break;
		}

		RakNet::BitStream bstream;
		if (timeStamp != 0)
		{
			bstream.Write((RakNet::MessageID) ID_TIMESTAMP);
			bstream.Write(timeStamp);
		}
		
		bstream.Write((RakNet::MessageID) ID_BROADCAST_FROM_ROOM);
		bstream.Write((std::uint8_t) client->joined_slot);
		bstream.Write(in);

		for (auto it = room->slots.begin(); it != room->slots.end(); it++) {
			if (*it != Client::UNASSIGNED_ID) {
				BIRIBIT_ASSERT(m_clients[*it] != nullptr);
				m_peer->Send(&bstream, HIGH_PRIORITY, reliability, room->id & 0xFF, m_clients[*it]->addr, false);
			}
		}
	}
}

void RakNetServer::SendRoomEntry(RakNet::SystemAddress addr, RakNet::BitStream& in)
{
	unique<Client>& client = GetClient(addr);
	if (client->joined_room > 0)
	{
		BIRIBIT_ASSERT(m_rooms[client->joined_room] != nullptr);
		BIRIBIT_ASSERT(m_rooms[client->joined_room]->slots[client->joined_slot] == client->id);
		unique<Room>& room = m_rooms[client->joined_room];

		std::size_t size = BITS_TO_BYTES(in.GetNumberOfUnreadBits());
		Room::Entry newEntry;
		newEntry.from_slot = client->joined_slot;
		newEntry.data.resize(size + 1);
		newEntry.data[size] = '\0';
		in.Read(&newEntry.data[0], size);
		room->journal.push_back(newEntry);

		Proto::RoomEntriesStatus proto_entries;
		PopulateProtoRoomEntriesStatus(room, &proto_entries);
		{
			Proto::RoomEntry* proto_entry = proto_entries.add_entries();
			proto_entry->set_id(room->journal.size() - 1);
			proto_entry->set_from_slot(newEntry.from_slot);
			proto_entry->set_entry_data(newEntry.data);
		}	

		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_JOURNAL_ENTRIES_STATUS, proto_entries)) {
			for (auto it = room->slots.begin(); it != room->slots.end(); it++) {
				if (*it != Client::UNASSIGNED_ID) {
					BIRIBIT_ASSERT(m_clients[*it] != nullptr);
					m_peer->Send(&bstream, MEDIUM_PRIORITY, RELIABLE, room->id & 0xFF, m_clients[*it]->addr, false);
				}
			}
		}
	}
}

void RakNetServer::RoomEntriesRequest(RakNet::SystemAddress addr, Proto::RoomEntriesRequest* proto_entriesReq)
{
	unique<Client>& client = GetClient(addr);
	if (client->joined_room > 0)
	{
		BIRIBIT_ASSERT(m_rooms[client->joined_room] != nullptr);
		BIRIBIT_ASSERT(m_rooms[client->joined_room]->slots[client->joined_slot] == client->id);
		unique<Room>& room = m_rooms[client->joined_room];

		Proto::RoomEntriesStatus proto_entries;
		PopulateProtoRoomEntriesStatus(room, &proto_entries);
		
		std::set<std::uint32_t> entriesSet;
		int size = proto_entriesReq->entries_id_size();
		for (int i = 0; i < size; i++)
		{
			std::uint32_t id = proto_entriesReq->entries_id(i);
			if (entriesSet.find(id) == entriesSet.end())
			{
				if (id > Room::Entry::UNASSIGNED_ID && id < room->journal.size())
				{
					Room::Entry& entry = room->journal[id];
					Proto::RoomEntry* proto_entry = proto_entries.add_entries();
					proto_entry->set_id(id);
					proto_entry->set_from_slot(entry.from_slot);
					proto_entry->set_entry_data(entry.data);
				}

				entriesSet.insert(id);
			}
		}

		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_JOURNAL_ENTRIES_STATUS, proto_entries))
			m_peer->Send(&bstream, MEDIUM_PRIORITY, RELIABLE, room->id & 0xFF, addr, false);
	}
}

void RakNetServer::PopulateProtoServerInfo(Proto::ServerInfo* proto_info)
{
	proto_info->set_name(m_name);
	proto_info->set_password_protected(m_passwordProtected);
	proto_info->set_max_clients(m_maxClients);
	proto_info->set_connected_clients(m_clientAddrMap.size());
}

void RakNetServer::PopulateProtoClient(unique<Client>& client, Proto::Client* proto_client)
{
	proto_client->set_id(client->id);
	proto_client->set_name(client->name);
	proto_client->set_appid(client->appid);
}

void RakNetServer::PopulateProtoRoom(unique<Room>& room, Proto::Room* proto_room)
{
	proto_room->set_id(room->id);
	for (std::size_t i = 0; i < room->slots.size(); i++)
		proto_room->add_joined_id_client(room->slots[i]);

	proto_room->set_journal_entries_count(room->journal.size());
}

void RakNetServer::PopulateProtoRoomJoin(unique<Client>& client, Proto::RoomJoin* proto_join)
{
	proto_join->set_id(client->joined_room);
	if (client->joined_room != Room::UNASSIGNED_ID)
		proto_join->set_slot_to_join(client->joined_slot);
}

void RakNetServer::PopulateProtoRoomEntriesStatus(unique<Room>& room, Proto::RoomEntriesStatus* proto_entries)
{
	proto_entries->set_room_id(room->id);
	BIRIBIT_ASSERT(room->journal.size() > 0);
	proto_entries->set_journal_size(room->journal.size() -1 );
}


void RakNetServer::RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data)
{
	if (data != nullptr && static_cast<RakNetServer*>(data)->m_peer == peer) {
		static_cast<RakNetServer*>(data)->RakNetUpdated();
	}
}

void RakNetServer::RakNetUpdated()
{
	m_pool->enqueue([this]() {
		RakNet::Packet* p = nullptr;
		while (m_peer != nullptr && (p = m_peer->Receive()) != nullptr) {
			HandlePacket(p);
			m_peer->DeallocatePacket(p);
		}
	});
}

void RakNetServer::HandlePacket(RakNet::Packet* p)
{
	RakNet::BitStream stream(p->data, p->length, false);
	RakNet::MessageID packetIdentifier;
	RakNet::Time timeStamp = 0;

	stream.Read(packetIdentifier);
	if (packetIdentifier == ID_TIMESTAMP)
	{
		BIRIBIT_ASSERT(p->length > (sizeof(RakNet::MessageID) + sizeof(RakNet::Time)));
		stream.Read(timeStamp);
		stream.Read(packetIdentifier);
	}

	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		printLog("Client %s disconnected.", p->systemAddress.ToString());
		RemoveClient(p->systemAddress);
		break;
	case ID_NEW_INCOMING_CONNECTION:
	{
		Client::id_t id = NewClient(p->systemAddress);
		printLog("New client(%d) \"%s\" connected from %s.", id, m_clients[id]->name.c_str(), p->systemAddress.ToString());
		break;
	}
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		stream.Read(packetIdentifier);
		printLog("%s has Raknet incompatible protocol version. Remote peer version: %d", p->systemAddress.ToString(), packetIdentifier);
		break;
	case ID_ADVERTISE_SYSTEM:
	{
		stream.Read(packetIdentifier);
		if (packetIdentifier == ID_SERVER_INFO_REQUEST)
		{
			Proto::ServerInfo proto_info;
			PopulateProtoServerInfo(&proto_info);
			RakNet::BitStream bstream;
			if (WriteMessage(bstream, ID_SERVER_INFO_RESPONSE, proto_info))
				m_peer->AdvertiseSystem(
					p->systemAddress.ToString(false),
					p->systemAddress.GetPort(),
					(const char*)bstream.GetData(),
					bstream.GetNumberOfBytesUsed());
		}
		break;
	}
	case ID_CONNECTION_LOST:
		printLog("ID_CONNECTION_LOST %s", p->systemAddress.ToString());
		RemoveClient(p->systemAddress);
		break;
	case ID_ERROR_CODE:
		BIRIBIT_WARN("Nothing to do with ID_ERROR_CODE");
		break;
	case ID_SERVER_INFO_REQUEST:
	{
		Proto::ServerInfo proto_info;
		PopulateProtoServerInfo(&proto_info);
		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_SERVER_INFO_RESPONSE, proto_info))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, false);
		
		break;
	}
	case ID_SERVER_INFO_RESPONSE:
		BIRIBIT_WARN("Nothing to do with ID_SERVER_INFO_RESPONSE");
		break;
	case ID_CLIENT_SELF_STATUS:
		BIRIBIT_WARN("Nothing to do with ID_CLIENT_SELF_STATUS");
		break;
	case ID_SERVER_STATUS_REQUEST:
	{
		Proto::ServerStatus proto_status;
		for (std::size_t i = 0; i < m_clients.size(); i++)
		{
			if (m_clients[i] != nullptr)
			{
				Proto::Client* proto_client = proto_status.add_clients();
				PopulateProtoClient(m_clients[i], proto_client);
			}
		}

		RakNet::BitStream bstream;
		if (WriteMessage(bstream, ID_SERVER_STATUS_RESPONSE, proto_status))
			m_peer->Send(&bstream, LOW_PRIORITY, RELIABLE_ORDERED, 0, p->systemAddress, false);

		break;
	}
	case ID_SERVER_STATUS_RESPONSE:
		BIRIBIT_WARN("Nothing to do with ID_SERVER_STATUS_RESPONSE");
		break;
	case ID_CLIENT_UPDATE_STATUS:
	{
		Proto::ClientUpdate proto_update;
		if (ReadMessage(proto_update, stream))
			UpdateClient(p->systemAddress, &proto_update);
		break;
	}
	case ID_CLIENT_STATUS_UPDATED:
		BIRIBIT_WARN("Nothing to do with ID_SERVER_CLIENT_NAME_CHANGED");
		break;
	case ID_CLIENT_DISCONNECTED:
		BIRIBIT_WARN("Nothing to do with ID_CLIENT_DISCONNECTED");
		break;
	case ID_ROOM_LIST_REQUEST:
		ListRooms(p->systemAddress);
		break;
	case ID_ROOM_LIST_RESPONSE:
		BIRIBIT_WARN("Nothing to do with ID_ROOM_LIST_RESPONSE");
		break;
	case ID_ROOM_CREATE_REQUEST:
	{
		Proto::RoomCreate proto_create;
		if (ReadMessage(proto_create, stream))
			CreateRoom(p->systemAddress, &proto_create);
		break;
	}
	case ID_ROOM_JOIN_RANDOM_OR_CREATE_REQUEST:
	{
		Proto::RoomCreate proto_create;
		if (ReadMessage(proto_create, stream))
			JoinRandomOrCreate(p->systemAddress, &proto_create);
		break;
	}
	case ID_ROOM_JOIN_REQUEST:
	{
		Proto::RoomJoin proto_join;
		if (ReadMessage(proto_join, stream))
			JoinRoom(p->systemAddress, &proto_join);
		break;
	}
	case ID_ROOM_STATUS:
		BIRIBIT_WARN("Nothing to do with ID_ROOM_STATUS");
		break;
	case ID_ROOM_JOIN_RESPONSE:
		BIRIBIT_WARN("Nothing to do with ID_ROOM_JOIN_RESPONSE");
		break;
	case ID_SEND_BROADCAST_TO_ROOM:
		SendRoomBroadcast(p->systemAddress, timeStamp, stream);
		break;
	case ID_BROADCAST_FROM_ROOM:
		BIRIBIT_WARN("Nothing to do with ID_BROADCAST_FROM_ROOM");
		break;
	case ID_JOURNAL_ENTRIES_REQUEST:
	{
		Proto::RoomEntriesRequest proto_entriesReq;
		if (ReadMessage(proto_entriesReq, stream))
			RoomEntriesRequest(p->systemAddress, &proto_entriesReq);
		break;
	}
	case ID_JOURNAL_ENTRIES_STATUS:
		BIRIBIT_WARN("Nothing to do with ID_JOURNAL_ENTRIES_STATUS");
		break;
	case ID_SEND_ENTRY_TO_ROOM:
		SendRoomEntry(p->systemAddress, stream);
		break;
	default:
		break;
	}
}

bool RakNetServer::WriteMessage(RakNet::BitStream& bstream,
	RakNet::MessageID msgId,
	::google::protobuf::MessageLite& msg)
{
	std::size_t size = (std::size_t) msg.ByteSize();
	m_buffer.Ensure(size);
	if (msg.SerializeToArray(m_buffer.data, m_buffer.size))
	{
		bstream.Write((RakNet::MessageID) msgId);
		bstream.Write(m_buffer.data, size);
		return true;
	}
	else
		BIRIBIT_WARN("%s unable to serialize.", msg.GetTypeName().c_str());

	return false;
}

template<typename T> bool RakNetServer::ReadMessage(T& msg, RakNet::BitStream& bstream)
{
	std::size_t size = BITS_TO_BYTES(bstream.GetNumberOfUnreadBits());
	m_buffer.Ensure(size);
	bstream.Read(m_buffer.data, size);
	return msg.ParseFromArray(m_buffer.data, size);
}

void RakNetServer::SendErrorCode(std::uint32_t error_code, RakNet::AddressOrGUID systemIdentifier)
{
	RakNet::BitStream tosend;
	tosend.Write((RakNet::MessageID) ID_ERROR_CODE);
	tosend.Write(error_code);
	m_peer->Send(&tosend, LOW_PRIORITY, RELIABLE, 0, systemIdentifier, false);
}


bool RakNetServer::Run(unsigned short _port, const char* _name, const char* _password, unsigned int maxClients)
{
	if (m_peer != nullptr) {
		return true;
	}
		
	m_peer = RakNet::RakPeerInterface::GetInstance();
	m_peer->SetTimeoutTime(10000, RakNet::UNASSIGNED_SYSTEM_ADDRESS);

	m_passwordProtected = (_password != NULL);
	if (m_passwordProtected)
		m_peer->SetIncomingPassword(_password, (int)strlen(_password));
		
	if (_port == 0)
		_port = SERVER_DEFAULT_PORT;

	printLog("Starting server. Server port: %d", _port);

	RakNet::SocketDescriptor socketDescriptors[2];
	socketDescriptors[0].port = _port;
	socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4
	socketDescriptors[1].port = _port;
	socketDescriptors[1].socketFamily = AF_INET6; // Test out IPV6

	if (maxClients == 0)
		maxClients = SERVER_DEFAULT_MAX_CONNECTIONS;

	bool bOk = m_peer->Startup(maxClients, socketDescriptors, 2) == RakNet::RAKNET_STARTED;
	m_peer->SetMaximumIncomingConnections(maxClients);
	if (!bOk)
	{
		printLog("Failed to start dual IPV4 and IPV6 ports. Trying IPV4 only.");

		bool bOk = m_peer->Startup(maxClients, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
		if (!bOk)
		{
			printLog("Server failed to start.  Terminating.");
			Close();
			return false;
		}
		else
		{
			printLog("IPV4 only success.");
		}
	}

	m_maxClients = maxClients;

	m_peer->SetOccasionalPing(true);
	m_peer->SetUnreliableTimeout(1000);

	std::stringstream log;
	m_peer->GetLocalIP(0);
	log << "Network interface(s) detected: " << m_peer->GetNumberOfAddresses() << std::endl;
	for (unsigned int i = 0; i < m_peer->GetNumberOfAddresses(); i++) {
		RakNet::SystemAddress sa = m_peer->GetInternalID(RakNet::UNASSIGNED_SYSTEM_ADDRESS, i);
		log << " " << i + 1 << ". " << sa.ToString(false) << " (LAN=" << sa.IsLANAddress() << ")" << std::endl;
	}
	printLog(log.str().c_str());

	if (_name == NULL) 
	{
		using clock = std::chrono::system_clock;
		auto timer = clock::now();
		auto now_c = clock::to_time_t(timer);

		int nameID = now_c % sizeof_string_array(randomNames);
		m_name = randomNames[nameID];
	}
	else
	{
		m_name = _name;
	}

	m_pool = std::unique_ptr<TaskPool>(new TaskPool(1, "RakNetServer"));
	m_peer->SetUserUpdateThread(RaknetThreadUpdate, this);

	printLog("Server \"%s\" running successfully", m_name.c_str());
	return true;
}

bool RakNetServer::isRunning()
{
	return m_peer != nullptr;
}

bool RakNetServer::Close()
{
	if (m_peer != nullptr)
	{
		m_peer->SetUserUpdateThread(RaknetThreadUpdate, nullptr);

		if (m_peer->IsActive()) {
			printLog("Server \"%s\" shutting down...", m_name.c_str());
			m_peer->Shutdown(60000);
		}
			
		printLog("Waiting for thread ends...");
		m_pool.reset(nullptr);

		RakNet::RakPeerInterface::DestroyInstance(m_peer);
		m_peer = nullptr;

		return true;
	}

	return false;
}
