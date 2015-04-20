#include "Commands.h"
#include <algorithm>
#include <functional>
#include <memory>
#include <iostream>

#include <Config.h>
#include <Types.h>

#include "BiribitClient.h"
#include "BiribitClientExports.h"

namespace Client
{

class ClientUpdater : public UpdaterListener
{
	shared<BiribitClient> client;
	Console* console;

	void OnUpdate(float deltaTime) override
	{

	}

	void OnGUI() override
	{

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
		if (client != nullptr)
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
