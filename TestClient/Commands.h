#ifndef COMMANDS_H
#define COMMANDS_H

#include <vector>

#include "Console.h"
#include "Updater.h"

class CommandHandler
{
	virtual void AddCommands(Console &console, Updater& updater) = 0;
	virtual void AddLogCallback(Console &console) {};
	virtual void DelLogCallback() {};

	static std::vector<CommandHandler*> handlers;

public:

	CommandHandler();

	static void AddAllCommands(Console &console, Updater& updater);
	static void AddAllLogCallback(Console &console);
	static void DelAllLogCallback();

};

#endif // COMMANDS_H
