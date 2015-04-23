#include "RakNetServer.h"
#include "Config.h"
#include "Types.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <tclap/CmdLine.h>

RakNetServer server;

#ifdef SYSTEM_WINDOWS

#include <Windows.h>

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
	if (dwCtrlType == CTRL_CLOSE_EVENT)
	{
		server.Close();
	}

	return TRUE;
}
#endif

#ifdef SYSTEM_LINUX
#include <signal.h>
void TerminationHandler(int signum)
{
	server.Close();
}

#endif

int main(int argc, char** argv)
{
#ifdef SYSTEM_WINDOWS
	BOOL ret = SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);
#endif

#ifdef SYSTEM_LINUX
	if (signal(SIGINT, TerminationHandler) == SIG_IGN)
		signal(SIGINT, SIG_IGN);
	if (signal(SIGHUP, TerminationHandler) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGTERM, TerminationHandler) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
#endif

	try
	{
		TCLAP::CmdLine cmd("Biribit server", ' ', "0.1");

		TCLAP::ValueArg<std::string> nameArg0("n", "name", "Server name shown", false, "", "name");
		TCLAP::ValueArg<std::string> nameArg1("p", "port", "Port to bind", false, "", "port");

		cmd.add(nameArg0);
		cmd.add(nameArg1);
		cmd.parse(argc, argv);

		// Get the value parsed by each arg. 
		std::string name = nameArg0.getValue();
		std::string port = nameArg1.getValue();

		int iPort = 0;
		std::stringstream ssPort(port);
		ssPort >> iPort;

		if (server.Run(iPort, name.length() == 0 ? nullptr : name.c_str()))
		{
			char c;
			while (server.isRunning() && std::cin >> c)
			{
				if (c == 'q') {
					break;
				}
			}

			server.Close();
		}
	}
	catch (TCLAP::ArgException &e)
	{
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}

	return 0;
}

