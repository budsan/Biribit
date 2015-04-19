#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <map>

class Console
{
public:
	Console();

	struct Command
	{
		typedef bool (*Parser)(std::stringstream &in, std::stringstream &out, void* data);

		Command();
		Command(const std::string &name, Parser parser, void* data = NULL);
		Command(const Command &other);

		Command& operator=(const Command &other);

		std::string name;
		Parser parser;
		void* data;
	};

	bool addCommand(const Command &cmd);
	bool execute(const std::string &name, bool showLineCommand = true);

	void print(const std::string &msg);
	void print(const char *fmt, ...);

	void put(char key);
	void take(unsigned int pos);

	void autocomplete();

	void keyBackspace();
	void keyDelete();

	void keyHome();
	void keyEnd();

	bool executeFromFile(const char* path);
	void executeCurrentCommand();

	void historyUp();
	void historyDown();

	void cursorLeft();
	void cursorRight();

	int getCursorPos();

	void clear();

	void setPrompt(const std::string &prompt);
	const std::string &getPrompt();

	const std::list<std::string>& out();
	std::string editing();
	void setEditing(const char* text);

private:
	std::list<std::string> history;
	std::list<std::string>::iterator historyNav;

	std::map<std::string, Command> commands;
	std::list<std::string> output;

	std::string prompt;

protected:

	const std::list<std::string>::iterator& HistoryNav();
	int cursorPos;
};
