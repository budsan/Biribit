#include "ConsoleInterface.h"
#include "Commands.h"

#include "imgui.h"
#include "Game.h"

bool command_quit(std::stringstream &in, std::stringstream &out, void* data)
{
	Game* game = static_cast<Game*>(data);
	game->Close();
	return true;
}
//
// CONSOLEINTERFACE
///////////////////////////////////////////////////////////////////////////////

ConsoleInterface::ConsoleInterface() : 
	displayed(false), 
	scrolldown(false)
{
	setPrompt(">");
}

ConsoleInterface::~ConsoleInterface()
{
	
}

void ConsoleInterface::OnUpdate(float deltaTime)
{
	
}

void ConsoleInterface::OnGUI()
{
	ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiSetCond_FirstUseEver);
	if (!ImGui::Begin("Console", &displayed))
	{
		ImGui::End();
		return;
	}

	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() * 2));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
	const std::list<std::string>& output = out();
	std::list<std::string>::const_reverse_iterator it = output.rbegin();
	for (unsigned int i = 0; it != output.rend(); ++i, ++it)
	{
		const char* item = it->c_str();
		ImVec4 col(1, 1, 1, 1);
		if (strncmp(item, ">", 1) == 0) col = ImVec4(1.0f, 0.8f, 0.6f, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::TextUnformatted(item);
		ImGui::PopStyleColor();
	}

	if (scrolldown)
		ImGui::SetScrollPosHere();

	scrolldown = false;
	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::Separator();

	str0 = editing();
	str0.reserve(128);

	bool Enter = ImGui::InputText("##Input", &str0[0], str0.capacity(), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, &TextEditCallbackStub, (void*)this);
	setEditing(str0.c_str());
	if (Enter) {
		executeCurrentCommand();
		scrolldown = true;
	}

	if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
		ImGui::SetKeyboardFocusHere(-1); // Auto focus

	ImGui::End();
}

bool ConsoleInterface::OnGUIMenu()
{
	displayed ^= ImGui::Button("Console");
	return true;
}

void ConsoleInterface::SetDisplay(bool enable)
{
	displayed = enable;
}

void ConsoleInterface::ToggleDisplay()
{
	displayed = !displayed;
}

int ConsoleInterface::TextEditCallbackStub(ImGuiTextEditCallbackData* data)
{
	ConsoleInterface* console = (ConsoleInterface*) data->UserData;
	return console->TextEditCallback(data);
}

int ConsoleInterface::TextEditCallback(ImGuiTextEditCallbackData* data)
{
	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackCompletion:
	{
		autocomplete();
		scrolldown = true;
		data->BufDirty = true;
		data->CursorPos = cursorPos;

		break;
	}
	case ImGuiInputTextFlags_CallbackHistory:
	{
		auto nav = HistoryNav();
		if (data->EventKey == ImGuiKey_UpArrow)
			historyUp();
		else if (data->EventKey == ImGuiKey_DownArrow)
			historyDown();

		if (nav != HistoryNav())
		{
			data->BufDirty = true;
			data->CursorPos = cursorPos;
		}
	}
	default:
		cursorPos = data->CursorPos;
		break;
	}

	if (data->BufDirty) {
		strcpy_s(data->Buf, data->BufSize, editing().c_str());
		data->CursorPos = cursorPos;
	}


	return 0;
}


class ConsoleInterfaceCommands : public CommandHandler
{
	void AddCommands(Console &console, Updater& updater) override
	{
		Game* game = dynamic_cast<Game*>(&updater);
		if (game == nullptr)
			return;

		console.addCommand(Console::Command("quit", command_quit, &updater));
		console.addCommand(Console::Command("exit", command_quit, &updater));
		console.addCommand(Console::Command("close", command_quit, &updater));

		updater.AddUpdaterListener(&listener);
	};

	ConsoleInterface listener;

public:
	ConsoleInterface& ConsoleInstance() {
		return listener;
	}
};

ConsoleInterfaceCommands console_commands;

ConsoleInterface& ConsoleInstance()
{
	return console_commands.ConsoleInstance();
}
