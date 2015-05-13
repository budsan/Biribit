#include "Commands.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <iostream>

#include <Config.h>
#include <Types.h>

#include "BiribitClient.h"
#include "BiribitClientExports.h"

#ifdef ENABLE_IMGUI
#include "imgui.h"
#endif

namespace Client
{

class ClientUpdater : public UpdaterListener
{
	shared<BiribitClient> client;
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

		if (ImGui::Button("Discover in LAN"))
			client->DiscoverOnLan();

		if (ImGui::Button("Clear list of servers"))
			client->ClearDiscoverInfo();

		if (ImGui::Button("Refresh servers"))
			client->RefreshDiscoverInfo();

		ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 5.0f);	
		std::vector<const char*> server_listbox;
		std::vector<std::string> server_listbox_str;
		const std::vector<ServerInfo>& info = client->GetDiscoverInfo();
		for (auto it = info.begin(); it != info.end(); it++)
		{
			const ServerInfo& serverInfo = *it;
			server_listbox_str.push_back(it->name + ", ping " + std::to_string(it->ping) + (it->passwordProtected ? ". Password protected." : ". No password."));
			server_listbox.push_back(server_listbox_str.back().c_str());
		}

		static int server_listbox_current = 0;
		if (!server_listbox.empty())
		{
			if (server_listbox_current > server_listbox.size())
				server_listbox_current = 0;
			ImGui::ListBox("Servers", &server_listbox_current, &server_listbox[0], server_listbox.size(), 4);

			if (ImGui::Button("Connect"))
			{
				const ServerInfo& serverInfo = info[server_listbox_current];
				client->Connect(serverInfo.addr.c_str(), serverInfo.port);
			}
		}

		std::vector<const char*> connections_listbox;
		std::vector<std::string> connections_listbox_str;
		const std::vector<ServerConnection>& conns = client->GetConnections();
		for (auto it = conns.begin(); it != conns.end(); it++)
		{
			const ServerConnection& connection = *it;
			connections_listbox_str.push_back(std::to_string(it->id) + ": " + it->name);
			connections_listbox.push_back(connections_listbox_str.back().c_str());
		}

		static int connections_listbox_current = 0;
		if (!connections_listbox.empty())
		{
			if (connections_listbox_current > connections_listbox.size())
				connections_listbox_current = 0;
			ImGui::ListBox("Connections", &connections_listbox_current, &connections_listbox[0], connections_listbox.size(), 4);

			if (ImGui::Button("Disconnect"))
			{
				const ServerConnection& connection = conns[connections_listbox_current];
				client->Disconnect(connection.id);
			}

			if (ImGui::Button("Disconnect all"))
				client->Disconnect();
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

	shared<BiribitClient> GetClient()
	{
		if (client == nullptr)
			client = shared<BiribitClient>(new BiribitClient());

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
