#include "Commands.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <iostream>
#include <list>

#include <Biribit/Client.h>

#ifdef ENABLE_IMGUI
#include "imgui.h"
#endif

template<int N> int sizeof_array(const char (&s)[N]) { return N; }

char* BiribitErrorStrings[] = {
	"NO_ERROR",
	"WARN_CLIENT_NAME_IN_USE",
	"WARN_CANNOT_LIST_ROOMS_WITHOUT_APPID",
	"WARN_CANNOT_CREATE_ROOM_WITHOUT_APPID",
	"WARN_CANNOT_CREATE_ROOM_WITH_WRONG_SLOT_NUMBER",
	"WARN_CANNOT_CREATE_ROOM_WITH_TOO_MANY_SLOTS",
	"WARN_CANNOT_JOIN_WITHOUT_ROOM_ID",
	"WARN_CANNOT_JOIN_TO_UNEXISTING_ROOM",
	"WARN_CANNOT_JOIN_TO_OTHER_APP_ROOM",
	"WARN_CANNOT_JOIN_TO_OCCUPIED_SLOT",
	"WARN_CANNOT_JOIN_TO_INVALID_SLOT",
	"WARN_CANNOT_JOIN_TO_FULL_ROOM"
};

namespace Client
{

class ClientUpdater : public UpdaterListener
{
	std::shared_ptr<Biribit::Client> client;
	Console* console;
	bool displayed;

	char addr[128];
	char pass[128];
	char cname[128];
	char appid[128];
	char chat[1024];

	int port;
	int server_listbox_current;
	int connections_listbox_current;
	int rooms_listbox_current;
	int slots;
	int slots_to_join;

	std::vector<Biribit::ServerInfo> serverInfo;
	std::vector<Biribit::Connection> connections;

	struct ConnectionInfo
	{
		std::vector<Biribit::RemoteClient> remoteClients;
		std::vector<Biribit::Room> rooms;

		Biribit::Room::id_t joined_room;
		Biribit::Room::slot_id_t joined_room_slot;

		std::list<std::string> chats;
		std::list<std::string> entries;
	};

	std::vector<ConnectionInfo> connectionsInfo;

	Biribit::Connection::id_t connectionId;

	std::vector<const char*> server_listbox;
	std::vector<std::string> server_listbox_str;

	std::vector<const char*> connections_listbox;
	std::vector<std::string> connections_listbox_str;

	std::vector<const char*> rooms_listbox;
	std::vector<std::string> rooms_listbox_str;

	std::list<std::function<bool()>> tasks;
public:

	ClientUpdater()
		: client(nullptr)
		, console(nullptr)
		, port(0)
		, server_listbox_current(0)
		, connections_listbox_current(0)
		, rooms_listbox_current(0)
		, slots(4)
		, slots_to_join(0)
		, connectionId(Biribit::Connection::UNASSIGNED_ID)
	{
		strcpy(addr, "localhost");
		strcpy(pass, "");
		strcpy(cname, "ClientName");
		strcpy(appid, "app-client-test");
	}

private:

	template<class T, class U> std::unique_ptr<T> unique_ptr_cast(std::unique_ptr<U>& ptr)
	{
		return std::unique_ptr<T>(static_cast<T*>(ptr.release()));
	}

	template<class T> void PushFuture(T* dst, std::shared_future<T> src)
	{
		tasks.push_back([this, dst, src]() -> bool { return HandleFuture<T>(dst, src); });
	}

	template<class T> bool HandleFuture(T* dst, std::shared_future<T> src)
	{
		if (!src.valid())
			return true;

		if (src.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
			(*dst) = src.get();
			return true;
		}

		return false;
	}

	void UpdateEntries(Biribit::Connection::id_t id, Biribit::Room::id_t joinedRoom)
	{
		if (id >= connectionsInfo.size())
			connectionsInfo.resize(id + 1);

		ConnectionInfo& info = connectionsInfo[id];
		info.joined_room = joinedRoom;
		if (info.joined_room > Biribit::Room::UNASSIGNED_ID)
		{
			std::list<std::string>& connectionEntries = info.entries;
			std::uint32_t count = client->GetEntriesCount(connectionId);
			for (std::uint32_t i = connectionEntries.size() + 1; i <= count; i++)
			{
				const Biribit::Entry& entry = client->GetEntry(connectionId, i);
				bool isPrintable = true;
				const char* it = (const char*)entry.data.getData();
				const char* itEnd = it + entry.data.getDataSize();

				std::stringstream hex;
				for (; it != itEnd; it++)
				{
					static char hexTable[] = "0123456789ABCDEF";
					static char bytehex[3] = { '\0', '\0', '\0' };

					bool ok = ((it + 1) == itEnd && (*it) == '\0') || isprint(*it);
					isPrintable = ok && isPrintable;

					bytehex[0] = hexTable[(*it & 0xF0) >> 4];
					bytehex[1] = hexTable[(*it & 0x0F) >> 0];
					hex << bytehex;
				}

				if (entry.id > Biribit::Entry::UNASSIGNED_ID)
				{
					std::stringstream ss;
					ss << "[Player " << (std::uint32_t) entry.from_slot << "]: ";
					if (isPrintable)
						ss << (const char*)entry.data.getData();
					else
						ss << "binary: " << hex.str();

					connectionEntries.push_back(ss.str());
				}
				else
				{
					break;
				}
			}
		}
	}

	void OnUpdate(float deltaTime) override
	{
		if (client == nullptr)
			return;

		std::unique_ptr<Biribit::Event> evnt;
		while ((evnt = client->PullEvent()) != nullptr)
		{
			switch (evnt->type)
			{
			case Biribit::TYPE_NONE:
				if (console != nullptr)
					console->print("Received none event.");
				break;
			case Biribit::TYPE_ERROR:
			{
				auto error_evnt = unique_ptr_cast<Biribit::ErrorEvent>(evnt);
				if (console != nullptr)
					console->print("Error event %s.", BiribitErrorStrings[error_evnt->which]);
				break;
			}
			case Biribit::TYPE_SERVER_LIST:
				PushFuture(&serverInfo, client->GetServerList().share());
				if (console != nullptr)
					console->print("Server list changed.");
				break;
			case Biribit::TYPE_CONNECTION:
				PushFuture(&connections, client->GetConnections().share());
				
				break;
			case Biribit::TYPE_REMOTE_CLIENT:
			{
				auto rc_evnt = unique_ptr_cast<Biribit::RemoteClientEvent>(evnt);
				auto id = rc_evnt->connection;
				if (id >= connectionsInfo.size())
					connectionsInfo.resize(id + 1);

				switch (rc_evnt->typeRemoteClient)
				{
				case Biribit::RemoteClientEvent::UPDATE_SELF_CLIENT:
					if (console != nullptr) console->print("Local client updated ID: %d. Name: %s, AppId: %s.", rc_evnt->client.id, rc_evnt->client.name.c_str(), rc_evnt->client.appid.c_str());
					break;
				case Biribit::RemoteClientEvent::UPDATE_REMOTE_CLIENT:
					if (console != nullptr) console->print("Client updated ID: %d. Name: %s, AppId: %s.", rc_evnt->client.id, rc_evnt->client.name.c_str(), rc_evnt->client.appid.c_str());
					break;
				case Biribit::RemoteClientEvent::DISCONNECTION:
					if (console != nullptr) console->print("Client disconnected ID: %d. Name: %s, AppId: %s.", rc_evnt->client.id, rc_evnt->client.name.c_str(), rc_evnt->client.appid.c_str());
				default:
					break;
				}

				

				PushFuture(&connectionsInfo[id].remoteClients, client->GetRemoteClients(id).share());
				break;
			}
			case Biribit::TYPE_ROOM_LIST:
			{
				auto rl_evnt = unique_ptr_cast<Biribit::RoomListEvent>(evnt);
				auto id = rl_evnt->connection;
				if (id >= connectionsInfo.size())
					connectionsInfo.resize(id + 1);

				connectionsInfo[id].rooms = std::move(rl_evnt->rooms);

				if (console != nullptr)
					console->print("Room list updated!");

				break;
			}
				break;
			case Biribit::TYPE_JOINED_ROOM:
			{
				auto jr_evnt = unique_ptr_cast<Biribit::JoinedRoomEvent>(evnt);
				auto id = jr_evnt->connection;
				if (id >= connectionsInfo.size())
					connectionsInfo.resize(id + 1);

				ConnectionInfo& info = connectionsInfo[id];
				info.joined_room = jr_evnt->room_id;
				info.joined_room_slot = jr_evnt->slot_id;
				info.entries.clear();

				if (console != nullptr)
					console->print("Joined room %d, slot %d.", info.joined_room, info.joined_room_slot);

				break;
			}
			case Biribit::TYPE_BROADCAST:
			{
				std::unique_ptr<Biribit::BroadcastEvent> recv = unique_ptr_cast<Biribit::BroadcastEvent>(evnt);
				auto id = recv->connection;
				if (id >= connectionsInfo.size())
					connectionsInfo.resize(id + 1);

				std::stringstream ss;
				auto secs = recv->when / 1000;
				auto mins = secs / 60;
				ss << mins % 60 << ":" << secs % 60;
				ss << " [Room " << recv->room_id << ", Player " << (std::uint32_t) recv->slot_id << "]: " << (const char*)recv->data.getData();
				connectionsInfo[id].chats.push_back(ss.str());
				break;
			}
			case Biribit::TYPE_ENTRIES:
			{
				std::unique_ptr<Biribit::EntriesEvent> entry = unique_ptr_cast<Biribit::EntriesEvent>(evnt);
				UpdateEntries(entry->connection, entry->room_id);
				break;
			}
			}
		}

		for (auto it = tasks.begin(); it != tasks.end();)
			if ((*it)())
				it = tasks.erase(it);
			else
				it++;
	}

	void OnGUI() override
	{
		ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiSetCond_FirstUseEver);

		if (client == nullptr)
			GetClient();

		if (!ImGui::Begin("Client", &displayed))
		{
			ImGui::End();
			return;
		}

		ImGui::InputText("Address", addr, sizeof_array(addr));
		ImGui::InputInt("Port", &port);
		ImGui::InputText("Password", pass, sizeof_array(pass)); ImGui::SameLine();
		if (ImGui::Button("Connect"))
			client->Connect(addr, port, (pass[0] != '\0') ? pass : nullptr);

		if (ImGui::CollapsingHeader("Servers"))
		{

			if (ImGui::Button("Discover on LAN"))
				client->DiscoverServersOnLAN();

			ImGui::SameLine();
			if (ImGui::Button("Clear list of servers"))
				client->ClearServerList();

			ImGui::SameLine();
			if (ImGui::Button("Refresh servers"))
				client->RefreshServerList();

			const std::vector<Biribit::ServerInfo>& discover_info = this->serverInfo;
			server_listbox.clear();
			server_listbox_str.clear();
			for (auto it = discover_info.begin(); it != discover_info.end(); it++) {
				std::stringstream str;
				str << it->name << " (" << it->connected_clients << "/" << it->max_clients << "). ";
				str << it->addr << ". Ping: " << it->ping << ". ";
				if (it->passwordProtected)
					str << "Password protected.";

				server_listbox_str.push_back(str.str());
			}

			for (auto it = server_listbox_str.begin(); it != server_listbox_str.end(); it++)
				server_listbox.push_back(it->c_str());

			if (!server_listbox.empty())
			{
				if (server_listbox_current >= server_listbox.size())
					server_listbox_current = 0;

				ImGui::ListBox("Servers", &server_listbox_current, &server_listbox[0], server_listbox.size(), 4);

				if (ImGui::Button("Connect selected"))
				{
					const Biribit::ServerInfo& serverInfo = discover_info[server_listbox_current];
					client->Connect(serverInfo.addr.c_str(), serverInfo.port);
				}
			}
		}

		const std::vector<Biribit::Connection>& conns = this->connections;
		connections_listbox.clear();
		connections_listbox_str.clear();
		for (auto it = conns.begin(); it != conns.end(); it++)
			connections_listbox_str.push_back(std::to_string(it->id) + ": " + it->name + ". Ping: " + std::to_string(it->ping));

		for (auto it = connections_listbox_str.begin(); it != connections_listbox_str.end(); it++)
			connections_listbox.push_back(it->c_str());


		if (connections_listbox.empty())
		{
			connectionId = Biribit::Connection::UNASSIGNED_ID;
		}
		else
		{
			if (connections_listbox_current >= connections_listbox.size())
				connections_listbox_current = 0;

			const Biribit::Connection& connection = conns[connections_listbox_current];
			connectionId = connection.id;
			if (ImGui::CollapsingHeader("Connections"))
			{
				ImGui::ListBox("Connections", &connections_listbox_current, &connections_listbox[0], connections_listbox.size(), 4);
				if (ImGui::Button("Disconnect"))
					client->Disconnect(connectionId);

				ImGui::SameLine();
				if (ImGui::Button("Disconnect all"))
					client->Disconnect();
			}

			if (connectionId >= connectionsInfo.size())
				connectionsInfo.resize(connectionId + 1);

			ConnectionInfo& info = connectionsInfo[connectionId];
			const std::vector<Biribit::RemoteClient>& clients = info.remoteClients;
			std::map<Biribit::RemoteClient::id_t, std::vector<Biribit::RemoteClient>::const_iterator> idToClient;
			Biribit::RemoteClient::id_t selfId = client->GetLocalClientId(connectionId);
			for (auto it = clients.begin(); it != clients.end(); it++)
				idToClient[it->id] = it;

			if (ImGui::CollapsingHeader("Clients"))
			{
				ImGui::InputText("Client name", cname, sizeof_array(cname));
				ImGui::InputText("Application Id", appid, sizeof_array(appid));
				if (ImGui::Button("Set name and appid"))
					client->SetLocalClientParameters(connectionId, Biribit::ClientParameters(cname, appid));

				ImGui::LabelText("##ClientsLabel", "Connected clients");
				ImGui::Separator();
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
				for (auto it = clients.begin(); it != clients.end(); it++) {
					idToClient[it->id] = it;
					std::string item = std::to_string(it->id) + ": " + it->name + ((it->id == selfId) ? " (YOU)" : "");
					ImGui::TextUnformatted(item.c_str());
				}
				ImGui::PopStyleVar();
			}
			

			const std::vector<Biribit::Room>& rooms = info.rooms;
			Biribit::Room::id_t joined = client->GetJoinedRoomId(connectionId);
			std::uint32_t slot = client->GetJoinedRoomSlot(connectionId);

			rooms_listbox.clear();
			rooms_listbox_str.clear();
			for (auto it = rooms.begin(); it != rooms.end(); it++) {
				std::stringstream ss;
				ss << "Room " << it->id << ". Slots: ";
				for (std::size_t i = 0; i < it->slots.size(); i++)
					ss << "(" << ((it->slots[i] == Biribit::RemoteClient::UNASSIGNED_ID) ? "Free" : idToClient[it->slots[i]]->name) << ") ";

				if (it->id == joined)
					ss << "Joined: " << slot;

				rooms_listbox_str.push_back(ss.str());
			}

			for (auto it = rooms_listbox_str.begin(); it != rooms_listbox_str.end(); it++)
				rooms_listbox.push_back(it->c_str());

			if (ImGui::CollapsingHeader("Rooms"))
			{
				ImGui::PushItemWidth(100);
				ImGui::InputInt("Slots count", &slots); ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Create Room"))
					client->CreateRoom(connectionId, slots);

				ImGui::PushItemWidth(100);
				ImGui::InputInt("Joining slot", &slots_to_join); ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Create & Slot"))
					client->CreateRoom(connectionId, slots, slots_to_join);

				if (ImGui::Button("Join Random Or Create"))
					client->JoinRandomOrCreateRoom(connectionId, slots);

				if (!rooms_listbox.empty())
				{
					if (rooms_listbox_current >= rooms_listbox.size())
						rooms_listbox_current = 0;

					ImGui::ListBox("Rooms", &rooms_listbox_current, &rooms_listbox[0], rooms_listbox.size(), 4);
					if (ImGui::Button("Refresh rooms"))
						client->RefreshRooms(connectionId);

					ImGui::SameLine();
					if (ImGui::Button("Join"))
						client->JoinRoom(connectionId, rooms[rooms_listbox_current].id);

					ImGui::SameLine();
					if (ImGui::Button("Leave"))
						client->JoinRoom(connectionId, 0);

					ImGui::InputInt("Slot", &slots_to_join); ImGui::SameLine();
					if (ImGui::Button("Join slot"))
						client->JoinRoom(connectionId, rooms[rooms_listbox_current].id, slots_to_join);
				}
				else
				{
					if (ImGui::Button("Refresh rooms"))
						client->RefreshRooms(connectionId);
				}
			}

			if (ImGui::CollapsingHeader("Chat"))
			{
				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);
				bool Enter = ImGui::InputText("##chat", chat, sizeof_array(chat), ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::PopItemWidth(); ImGui::SameLine();
				if (ImGui::Button("Send") || Enter)
				{
					Biribit::Packet packet;
					packet.append(chat, strlen(chat) + 1);
					client->SendBroadcast(connectionId, packet);
					chat[0] = '\0';
				}

				ImGui::SameLine();
				if (ImGui::Button("As Entry"))
				{
					Biribit::Packet packet;
					packet.append(chat, strlen(chat) + 1);
					client->SendEntry(connectionId, packet);
					chat[0] = '\0';
				}

				if (ImGui::Button("Binary entry test"))
				{
					Biribit::Packet packet;
					packet << 0 << -1 << 0 << -1;
					client->SendEntry(connectionId, packet);
				}

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
				for (auto it = info.chats.rbegin(); it != info.chats.rend(); it++)
					ImGui::TextUnformatted(it->c_str());
				ImGui::PopStyleVar();
			}

			if (ImGui::CollapsingHeader("Entries"))
			{
				ImGui::LabelText("##entriesconn", "Connection %d. Room %d.", connectionId, info.joined_room);
				ImGui::Separator();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
				for (auto it = info.entries.begin(); it != info.entries.end(); it++)
						ImGui::TextUnformatted(it->c_str());
				ImGui::PopStyleVar();
			}
		}

	
		ImGui::End();
	}

	int Priority() const override
	{
		return 1;
	}

public:

	void SetConsole(Console* _console) {
		console = _console;
	}

	Console* GetConsole() {
		return console;
	}

	std::shared_ptr<Biribit::Client> GetClient()
	{
		if (client == nullptr)
			client = std::shared_ptr<Biribit::Client>(new Biribit::Client());

		return client;
	}
};

ClientUpdater updater;

void STDCALL LogCallback(const char* msg)
{
	if (updater.GetConsole() != nullptr)
		updater.GetConsole()->print(msg);
}

bool command_lookforservers(std::stringstream &in, std::stringstream &out, void*)
{
	int port = 0;
	in >> port;

	if (port < 0 && port > 0xFFFF)
		port = 0;

	std::cerr << "DiscoverServersOnLAN" << std::endl;
	updater.GetClient()->DiscoverServersOnLAN(port);

	return true;
}

} //namespace Client

class ClientCommands : public CommandHandler
{
	void AddCommands(Console &console, Updater& updater) override
	{
		console.addCommand(Console::Command("lookforservers", Client::command_lookforservers, nullptr));
		updater.AddUpdaterListener(&Client::updater);
	}

	virtual void AddLogCallback(Console &console) override
	{
		Client::updater.SetConsole(&console);
		BiribitClientAddLogCallback(Client::LogCallback);
	}

	virtual void DelLogCallback() override
	{
		BiribitClientClean();
	}
};

ClientCommands client_commands;
