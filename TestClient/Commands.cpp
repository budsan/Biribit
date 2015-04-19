#include "Commands.h"

std::vector<CommandHandler*> CommandHandler::handlers;

CommandHandler::CommandHandler() {
	handlers.push_back(this);
}

void CommandHandler::AddAllCommands(Console &console, Updater& updater)
{
	for (auto it = handlers.begin() ; it != handlers.end(); it++)
		(*it)->AddCommands(console, updater);
}

void CommandHandler::AddAllLogCallback(Console &console)
{
	for (auto it = handlers.begin() ; it != handlers.end(); it++)
		(*it)->AddLogCallback(console);
}

void CommandHandler::DelAllLogCallback()
{
	for (auto it = handlers.begin() ; it != handlers.end(); it++)
		(*it)->DelLogCallback();
}

//---------------------------------------------------------------------------//

bool command_print(std::stringstream &in, std::stringstream &out, void* blah)
{
	(void) blah;
	std::string str;
	in >> str;
	out << str;

	while(std::getline(in, str))
		out << str;

	return true;
}

bool command_exec(std::stringstream &in, std::stringstream &out, void* console)
{
	(void) out;

	std::string path;
	in >> path;

	if (((Console*)console)->executeFromFile(path.c_str()))
	{
		return true;
	}
	else
	{
		out << "File " << path << " not found.";
	}

	return false;
}


//---------------------------------------------------------------------------//

class BasicCommands : public CommandHandler
{
	void AddCommands(Console &console, Updater& updater) override
	{
		console.addCommand(Console::Command("print", command_print));
		console.addCommand(Console::Command("exec", command_exec, &console));
	}
};

BasicCommands basic_commands;
