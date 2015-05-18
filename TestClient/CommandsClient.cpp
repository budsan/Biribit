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

	void OnUpdate(float deltaTime) override
	{

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

		static char addr[128] = "localhost";
		static char pass[128] = "";
		static int port = 0;

		ImGui::InputText("Address", addr, sizeof_array(addr));
		ImGui::InputInt("Port", &port); 
		ImGui::InputText("Password", pass, sizeof_array(pass)); ImGui::SameLine();
		if (ImGui::Button("Connect"))
			client->Connect(addr, port, (pass[0] != '\0') ? pass : nullptr);

		if (ImGui::Button("Discover in LAN"))
			client->DiscoverOnLan();

		if (ImGui::Button("Clear list of servers"))
			client->ClearDiscoverInfo();

		if (ImGui::Button("Refresh servers"))
			client->RefreshDiscoverInfo();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);	
		std::vector<const char*> server_listbox;
		std::vector<std::string> server_listbox_str;
		const std::vector<Biribit::ServerInfo>& info = client->GetDiscoverInfo();

		for (auto it = info.begin(); it != info.end(); it++)
			server_listbox_str.push_back(it->name + ", ping " + std::to_string(it->ping) + (it->passwordProtected ? ". Password protected." : ". No password."));
			
		for (auto it = server_listbox_str.begin(); it != server_listbox_str.end(); it++)
			server_listbox.push_back(it->c_str());

		static int server_listbox_current = 1;
		if (!server_listbox.empty())
		{
			if (server_listbox_current >= server_listbox.size())
				server_listbox_current = 0;

			ImGui::ListBox("Servers", &server_listbox_current, &server_listbox[0], server_listbox.size(), 4);

			if (ImGui::Button("Connect selected"))
			{
				const Biribit::ServerInfo& serverInfo = info[server_listbox_current];
				client->Connect(serverInfo.addr.c_str(), serverInfo.port);
			}
		}

		std::vector<const char*> connections_listbox;
		std::vector<std::string> connections_listbox_str;
		const std::vector<Biribit::ServerConnection>& conns = client->GetConnections();

		for (auto it = conns.begin(); it != conns.end(); it++)
			connections_listbox_str.push_back(std::to_string(it->id) + ": " + it->name + ". Ping: " + std::to_string(it->ping));

		for (auto it = connections_listbox_str.begin(); it != connections_listbox_str.end(); it++)
			connections_listbox.push_back(it->c_str());

		static int connections_listbox_current = 0;
		if (!connections_listbox.empty())
		{
			if (connections_listbox_current >= connections_listbox.size())
				connections_listbox_current = 0;

			ImGui::ListBox("Connections", &connections_listbox_current, &connections_listbox[0], connections_listbox.size(), 4);
			if (ImGui::Button("Disconnect selected"))
			{
				const Biribit::ServerConnection& connection = conns[connections_listbox_current];
				client->Disconnect(connection.id);
			}

			if (ImGui::Button("Disconnect all"))
				client->Disconnect();

			static char cname[128] = "ClientName";
			static char appid[128] = "app-client-test";

			ImGui::InputText("Client name", cname, sizeof_array(cname));
			ImGui::InputText("Application Id", appid, sizeof_array(appid));
			if (ImGui::Button("Set on selected connection"))
				client->SetLocalClientParameters(conns[connections_listbox_current].id, Biribit::ClientParameters(cname, appid));

			ImGui::LabelText("##ClientsLabel", "Connected clients");
			ImGui::Separator();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
			const std::vector<Biribit::RemoteClient>& clients = client->GetRemoteClients(conns[connections_listbox_current].id);
			Biribit::RemoteClient::id_t selfId = client->GetLocalClientId(conns[connections_listbox_current].id);
			for (auto it = clients.begin(); it != clients.end(); it++) {
				std::string item = std::to_string(it->id) + ": " + it->name + ( (it->id == selfId) ? " (YOU)" : "");
				ImGui::TextUnformatted(item.c_str());
			}
			ImGui::PopStyleVar();
		}

		ImGui::PopStyleVar();

		ImGui::End();
	}

	int Priority() const override
	{
		return 1;
	}

public:

	ClientUpdater() : client(nullptr), console(nullptr)
	{

	}

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

bool command_scanlan(std::stringstream &in, std::stringstream &out, void*)
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
		console.addCommand(Console::Command("scanlan", Client::command_scanlan, nullptr));
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
