#include "Commands.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <iostream>

#include <Biribit/Client.h>

#ifdef ENABLE_IMGUI
#include "imgui.h"
#endif

template<int N> int sizeof_array(const char (&s)[N]) { return N; }

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
	Biribit::ServerConnection::id_t connectionId;

	std::vector<const char*> server_listbox;
	std::vector<std::string> server_listbox_str;

	std::vector<const char*> connections_listbox;
	std::vector<std::string> connections_listbox_str;

	std::vector<const char*> rooms_listbox;
	std::vector<std::string> rooms_listbox_str;

	std::map<Biribit::ServerConnection::id_t, std::list<std::string>> chats;
	std::map<Biribit::ServerConnection::id_t, std::pair<Biribit::Room::id_t, std::list<std::string>>> entries;
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
		, connectionId(Biribit::ServerConnection::UNASSIGNED_ID)
	{
		strcpy(addr, "localhost");
		strcpy(pass, "");
		strcpy(cname, "ClientName");
		strcpy(appid, "app-client-test");
	}

private:

	void OnUpdate(float deltaTime) override
	{
		if (client == nullptr)
			return;

		std::unique_ptr<Biribit::Received> recv;
		while ((recv = client->PullReceived()) != nullptr)
		{
			std::stringstream ss;
			auto secs = recv->when / 1000;
			auto mins = secs / 60;
			ss <<  mins % 60 << ":" << secs % 60;
			ss << " [Room " << recv->room_id << ", Player " << (std::uint32_t) recv->slot_id << "]: " << (const char*)recv->data.getData();
			chats[recv->connection].push_back(ss.str());
		}

		if (connectionId != Biribit::ServerConnection::UNASSIGNED_ID)
		{
			std::pair<Biribit::Room::id_t, std::list<std::string>>& pair = entries[connectionId];
			Biribit::Room::id_t joinedRoom = client->GetJoinedRoomId(connectionId);
			if (pair.first != joinedRoom) {
				pair.first = joinedRoom;
				pair.second.clear();
			}

			if (joinedRoom > Biribit::Room::UNASSIGNED_ID)
			{
				std::list<std::string>& connectionEntries = pair.second;
				std::uint32_t count = client->GetEntriesCount(connectionId);
				for (std::uint32_t i = connectionEntries.size() + 1; i <= count; i++)
				{
					const Biribit::Entry& entry = client->GetEntry(connectionId, i);
					if (entry.id > Biribit::Entry::UNASSIGNED_ID)
					{
						std::stringstream ss;
						ss << " [Player " << (std::uint32_t) entry.from_slot << "]: " << (const char*)entry.data.getData();
						connectionEntries.push_back(ss.str());
					}
					else
					{
						break;
					}
				}
			}
		}
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

			if (ImGui::Button("Discover in LAN"))
				client->DiscoverOnLan();

			ImGui::SameLine();
			if (ImGui::Button("Clear list of servers"))
				client->ClearDiscoverInfo();

			ImGui::SameLine();
			if (ImGui::Button("Refresh servers"))
				client->RefreshDiscoverInfo();

			const std::vector<Biribit::ServerInfo>& discover_info = client->GetDiscoverInfo();
			server_listbox.clear();
			server_listbox_str.clear();
			for (auto it = discover_info.begin(); it != discover_info.end(); it++)
				server_listbox_str.push_back(it->name + ". " + it->addr + ", ping " + std::to_string(it->ping) + (it->passwordProtected ? ".Password protected." : ".No password."));

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

		const std::vector<Biribit::ServerConnection>& conns = client->GetConnections();
		connections_listbox.clear();
		connections_listbox_str.clear();
		for (auto it = conns.begin(); it != conns.end(); it++)
			connections_listbox_str.push_back(std::to_string(it->id) + ": " + it->name + ". Ping: " + std::to_string(it->ping));

		for (auto it = connections_listbox_str.begin(); it != connections_listbox_str.end(); it++)
			connections_listbox.push_back(it->c_str());


		if (connections_listbox.empty())
		{
			connectionId = Biribit::ServerConnection::UNASSIGNED_ID;
		}
		else
		{
			if (connections_listbox_current >= connections_listbox.size())
				connections_listbox_current = 0;

			const Biribit::ServerConnection& connection = conns[connections_listbox_current];
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
			
			const std::vector<Biribit::RemoteClient>& clients = client->GetRemoteClients(connectionId);
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
			

			const std::vector<Biribit::Room>& rooms = client->GetRooms(connectionId);
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
					packet.append(chat, strlen(chat));
					client->SendBroadcast(connectionId, packet);
					chat[0] = '\0';
				}

				ImGui::SameLine();
				if (ImGui::Button("As Entry"))
				{
					Biribit::Packet packet;
					packet.append(chat, strlen(chat));
					client->SendEntry(connectionId, packet);
					chat[0] = '\0';
				}

				if (ImGui::Button("Entry test"))
				{
					Biribit::Packet packet;
					packet << 0 << 255 << 0 << 255;
					client->SendEntry(connectionId, packet);
				}

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
				for (auto it = chats[connectionId].rbegin(); it != chats[connectionId].rend(); it++)
					ImGui::TextUnformatted(it->c_str());
				ImGui::PopStyleVar();
			}
		}

		if (ImGui::CollapsingHeader("Entries"))
		{
			for (auto it = entries.begin(); it != entries.end(); it++)
			{
				auto pair = it->second;
				ImGui::LabelText("##entriesconn", "Connection %d. Room %d.", it->first, pair.first);
				ImGui::Separator();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
				for (auto strIt = pair.second.begin(); strIt != pair.second.end(); strIt++)
					ImGui::TextUnformatted(strIt->c_str());
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

	std::cerr << "DiscoverOnLan" << std::endl;
	updater.GetClient()->DiscoverOnLan(port);

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
