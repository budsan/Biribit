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

		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() * 2));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		const std::vector<ServerDescription>& info = client->GetDiscoverInfo();
		for (auto it = info.begin(); it != info.end(); it++)
		{
			const ServerDescription& serverInfo = *it;
			ImVec4 col(1, 1, 1, 1);
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::TextUnformatted((it->name + ", ping " + std::to_string(it->ping)).c_str());
			ImGui::PopStyleColor();
		}
		ImGui::PopStyleVar();
		ImGui::EndChild();

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
