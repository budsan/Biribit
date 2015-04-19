#include "Console.h"

#include <cstdarg>
#include <cstdlib>

Console::Command::Command()
	: name("blah"), parser(NULL), data(NULL)
{
}

Console::Command::Command(const std::string &name, Parser parser, void* data)
	: name(name), parser(parser), data(data)
{
}

Console::Command::Command(const Command &other)
	: name(other.name), parser(other.parser), data(other.data)
{
}

Console::Command& Console::Command::operator=(const Console::Command &other)
{
	name = other.name;
	parser = other.parser;
	data = other.data;
	return *this;
}


Console::Console()
{
	history.push_front(std::string());
	historyNav = history.begin();
	cursorPos = 0;
}

bool Console::addCommand(const Command &cmd)
{
	commands[cmd.name] = cmd;
	return true;
}

bool Console::execute(const std::string &cmd, bool showLineCommand)
{
	bool result = false;
	if (showLineCommand) print(prompt + " " + cmd);

	std::stringstream in(cmd);
	std::string command;
	if (in >> command)
	{

		std::stringstream out;
		std::map<std::string, Command>::iterator it = commands.find(command);
		if (it != commands.end())
		{
			it->second.parser(in, out, it->second.data);
			if (!out.str().empty()) {
				print(out.str());
			}

			result = true;
		}
		else
		{
			out << "Error: " << command << " is not a valid command";
			print(out.str());
		}
	}

	return result;
}

void Console::print(const std::string &msg)
{
	std::stringstream out(msg);
	if (!out.str().empty()) {
		std::string line;
		while(std::getline(out, line)) {
			if (!line.empty()) output.push_back(line);
		}
	}
}

void Console::print(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int size = 100;
	std::string str;
	while (1)
	{
		str.resize(size);
		int n = vsnprintf((char *)str.c_str(), size, fmt, ap);

		if (n > -1 && n < size) {
			str.resize(n);
			break;
		}

		if (n > -1) size = n + 1;
		else        size *= 2;
	}
	va_end(ap);

	print(str);
}

void Console::put(char key)
{
	if (key >= ' ' && key <= '~')
	{
		size_t len = historyNav->length();
		historyNav->push_back('\0');

		for (size_t i = len; i > cursorPos; --i) {
			(*historyNav)[i] = (*historyNav)[i-1];
		}

		(*historyNav)[cursorPos++] = key;
	}
}

void Console::take(unsigned int pos)
{
	if(pos >= 0 && pos < historyNav->length())
	{
		for (size_t i = pos; i < (historyNav->length()-1); ++i) {
			(*historyNav)[i] = (*historyNav)[i+1];
		}

		historyNav->resize(historyNav->size() - 1);
	}
}

std::map<std::string, Console::Command>::const_iterator FindPrefix(const std::map<std::string, Console::Command> &map, const std::string& search_for)
{
	std::map<std::string, Console::Command>::const_iterator it = map.lower_bound(search_for);
	if (it != map.end())
	{
		const std::string& key = it->first;
		if (key.compare(0, search_for.size(), search_for) == 0) {
			return it;
		}
	}

	return map.end();
}

int areEqualUntil(const std::string& s1, const std::string& s2)
{
    const char* p = s1.c_str();
    const char* q = s2.c_str();
    int i = 0;

    while (*p != '\0' && *q != '\0' && *p == *q) {
	i++; p++; q++;
    }

    return i;
}

void Console::autocomplete()
{
	int num = 0;
	std::stringstream in(*historyNav);
	std::string command;
	while(in >> command) num++;

	//only perform autocomplete with the first word
	if ( num < 2 )
	{
		int type = 0; // matches of command being a prefix of some command
		std::string autocomplete;
		std::map<std::string, Command>::const_iterator it = FindPrefix(commands, command);
		for (; it != commands.end(); it++)
		{
			const std::string& key = it->first;
			if (key.compare(0, command.size(), command) == 0)
			{
				if (type == 0) // first match
				{ 
					type = 1;
					autocomplete = it->second.name;
				}
				else if (type == 1) // second, we must show alternatives
				{
					std::stringstream current;
					current << prompt << " " << *historyNav;
					output.push_front(std::string(current.str()));
					output.push_front(std::string(autocomplete));
					output.push_front(std::string(it->second.name));

					// we still autocomplete until where both commands stops being equal
					int ind = areEqualUntil(autocomplete, it->second.name);
					autocomplete = autocomplete.substr(0, ind);
					type = 2;
				}
				else if (type == 2) { // n-match, write new match on the end
					output.push_front(std::string(it->second.name));
					int ind = areEqualUntil(autocomplete, it->second.name);
					autocomplete = autocomplete.substr(0, ind);
				}
			}
			else break;
		}

		if (type > 0) // if we have some match, we have autocomplete (multiple or unique)
		{
			std::string complete = std::string(autocomplete).substr(command.length());
			*historyNav += complete;
			cursorPos = historyNav->length();

			//here we have a unique autocomplete
			if (type == 1 && !complete.empty()) put(' ');
		}
	}
}

void Console::keyBackspace()
{
	if(cursorPos > 0) {
		take(--cursorPos);
	}
}

void Console::keyDelete()
{
	take(cursorPos);
}

void Console::keyHome()
{
	cursorPos = 0;
}

void Console::keyEnd()
{
	cursorPos = historyNav->length();
}


bool Console::executeFromFile(const char* path)
{
	std::ifstream fs(path);

	if (fs.is_open())
	{
		int line = 0;
		std::string command;

		while(std::getline(fs, command))
		{
			//ignore from # to end of line
			size_t pos = command.find('#');
			if (pos != std::string::npos)
				command = command.substr(0, pos);

			int num = 0;
			std::stringstream in(command);
			std::string params;
			if(in >> params) num++;

			if (num > 0)
			{
				if (!execute(command, false))
				{
					std::stringstream out;
					out << "Line " << line << " ";
					output.front() = out.str() + output.front();
				}
			}

			line++;
		}


		return true;
	}

	return false;
}

void Console::executeCurrentCommand()
{
	int num = 0;
	std::stringstream in(*historyNav);
	std::string params;
	if(in >> params) num++;

	if (num > 0)
	{
		execute(*historyNav);
		history.front() = *historyNav;
		history.push_front(std::string());
		historyNav = history.begin();
		cursorPos = historyNav->length();
	}
}

void Console::historyUp()
{
	historyNav++;

	if (historyNav == history.end()) {
		historyNav--;
	}
	else {
		cursorPos = historyNav->length();
	}
}

void Console::historyDown()
{
	if (historyNav != history.begin()) {
		historyNav--;
		cursorPos = historyNav->length();
	}
}

void Console::cursorLeft()
{
	if (cursorPos > 0) cursorPos--;
}

void Console::cursorRight()
{
	if (cursorPos < historyNav->length()) cursorPos++;
}

int Console::getCursorPos()
{
	return cursorPos;
}

const std::list<std::string>& Console::out()
{
	return output;
}

std::string Console::editing()
{
	return *historyNav;
}

void Console::setEditing(const char* text)
{
	*historyNav = text;
}

void Console::clear()
{
	output.clear();
}

void Console::setPrompt(const std::string &_prompt)
{
	prompt = _prompt;
}

const std::string &Console::getPrompt()
{
	return prompt;
}

const std::list<std::string>::iterator& Console::HistoryNav()
{
	return historyNav;
}

