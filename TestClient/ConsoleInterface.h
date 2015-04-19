#pragma once

#include "Console.h"
#include "Updater.h"

struct ImGuiTextEditCallbackData;
class ConsoleInterface : public Console, public UpdaterListener
{
public:
	virtual ~ConsoleInterface();

	void OnUpdate(float deltaTime);
	void OnGUI();
	bool OnGUIMenu();

	void SetDisplay(bool enable);
	void ToggleDisplay();

private:
	ConsoleInterface();

	static int TextEditCallbackStub(ImGuiTextEditCallbackData* data);
	int TextEditCallback(ImGuiTextEditCallbackData* data);

	bool displayed;
	bool scrolldown;

	std::string str0;

	friend class ConsoleInterfaceCommands;
};


ConsoleInterface& ConsoleInstance();