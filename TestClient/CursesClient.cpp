#include <curses.h>
#include <string.h>

#include <sstream>
#include <iostream>

#include <chrono>
#include <thread>
#include <mutex>

#include "Commands.h"

#include <signal.h>



class TerminalUpdater : public Updater
{
	typedef std::chrono::time_point<std::chrono::system_clock> TimePoint;
	TimePoint m_clock;

public:
	
	TerminalUpdater() {
		m_clock = std::chrono::system_clock::now();
	}

	void Update()
	{
		TimePoint now = std::chrono::system_clock::now();
		std::chrono::duration<float> delta = now - m_clock;
		UpdateListeners(delta.count());
		m_clock = now;
	}
};

Console console;
TerminalUpdater updater;
std::mutex _mutex;

bool _exit = false;
bool command_exit(std::stringstream &in, std::stringstream &out, void* blah)
{
	(void) in; (void) out; (void) blah;

	_exit = true;
	return true;
}

void UpdateThread()
{
	while(!_exit)
	{
		{
			std::lock_guard<std::mutex> lock(_mutex);
			updater.Update();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void TerminationHandler(int signum)
{
	_exit = true;	
}

int main(int argc, char** argv)
{
	(void) argc;
	(void) argv;
	
	if (signal(SIGINT, TerminationHandler) == SIG_IGN)
		signal(SIGINT, SIG_IGN);
	if (signal(SIGHUP, TerminationHandler) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGTERM, TerminationHandler) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);

	console.setPrompt(">");
	AddTestingCommands(console, updater);
	AddLogCallback(console);
	console.addCommand(Console::Command("exit", command_exit));
	console.print(std::string("Client Terminal Test"));

	int row, col;
	int c = 0;

	initscr();
	noecho();
	cbreak();
	keypad(stdscr, FALSE);

	timeout(100);
	getmaxyx(stdscr, row, col);

	std::thread thread(&UpdateThread);

	while(!_exit)
	{
		{
			std::lock_guard<std::mutex> lock(_mutex);
			clear();
			int curr = row - 1;

			std::stringstream current;
			current << console.getPrompt() << " " << console.editing() << ' ';
			std::string edit = current.str();
			edit = edit.length() < (size_t) col ? edit : edit.substr(0, col);
			mvprintw(curr--, 0, edit.c_str());


			const std::list<std::string>& output = console.out();
			std::list<std::string>::const_iterator it = output.begin();
			for(; curr >= 0 && it != output.end(); curr--, it++) {
				std::string op = it->length() < (size_t) col ? *it : it->substr(0, col);
				mvprintw(curr, 0, op.c_str());
			}

			move(row - 1, std::min((int) (console.getPrompt().length() + 1 + console.getCursorPos()), col-2));
		}

		c = getch();

		{
			std::lock_guard<std::mutex> lock(_mutex);
			switch(c)
			{
			case KEY_UP:
				console.historyUp();
				break;
			case KEY_DOWN:
				console.historyDown();
				break;
			case KEY_LEFT:
				console.cursorLeft();
				break;
			case KEY_RIGHT:
				console.cursorRight();
				break;
			case KEY_RESIZE:
				getmaxyx(stdscr, row, col);
				break;
			case KEY_BACKSPACE:
			case 8:
			case 127:
				console.keyBackspace();
				break;
			case KEY_DC:
				console.keyDelete();
				break;
			case KEY_EOL:
			case KEY_EOS:
				console.keyEnd();
				break;
			case KEY_HOME:
				console.keyHome();
				break;
			case KEY_ENTER:
			case '\n':
				console.executeCurrentCommand();
				break;
			case KEY_STAB:
			case '\t':
				console.autocomplete();
				break;
			case KEY_MOUSE:
				break;
			default:
				{
					//std::stringstream out;
					//out << (int) c << std::endl;
					//console.print(out.str());
					console.put((char) c);
					break;
				}
			}
		}
	}

	endwin();

	if (thread.joinable())
		thread.join();

	DelLogCallback();

	delete ElabClient::getInst();

	return 0;
}
