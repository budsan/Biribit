#include <Biribit/Server/RakNetServer.h>
#include <Biribit/Common/PrintLog.h>
#include <Biribit/BiribitConfig.h>
#include <Biribit/Common/Types.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>

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

// Found here:
// http://stackoverflow.com/questions/17954432/creating-a-daemon-in-linux

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

static void skeleton_daemon()
{
	pid_t pid;

	/* Fork off the parent process */
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* On success: The child process becomes session leader */
	if (setsid() < 0)
		exit(EXIT_FAILURE);

	/* Catch, ignore and handle signals */
	//TODO: Implement a working signal handler */
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0)
		exit(EXIT_FAILURE);

	/* Success: Let the parent terminate */
	if (pid > 0)
		exit(EXIT_SUCCESS);

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");

	/* Close all open file descriptors */
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
	{
		close(x);
	}

	/* Open the log file */
	openlog("BiribitServer", LOG_PID, LOG_DAEMON);
}

void STDCALL PrintDaemonLog(const char* msg)
{
	syslog(LOG_NOTICE, msg);
}

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

	skeleton_daemon();
	Log_AddCallback(PrintDaemonLog);
#endif

	

	try
	{
		TCLAP::CmdLine cmd("Biribit server", ' ', "0.1");

		TCLAP::ValueArg<std::string> nameArg0("n", "name", "Server name shown", false, "", "name");
		TCLAP::ValueArg<std::string> nameArg1("p", "port", "Port to bind", false, "", "port");
		TCLAP::ValueArg<std::string> nameArg2("w", "password", "Server password", false, "", "password");

		cmd.add(nameArg0);
		cmd.add(nameArg1);
		cmd.add(nameArg2);
		cmd.parse(argc, argv);

		// Get the value parsed by each arg. 
		std::string name = nameArg0.getValue();
		std::string port = nameArg1.getValue();
		std::string pass = nameArg2.getValue();

		int iPort = 0;
		std::stringstream ssPort(port);
		ssPort >> iPort;

		if (server.Run(iPort, name.empty() ? nullptr : name.c_str(), pass.empty() ? nullptr : pass.c_str()))
		{
			char c;
			while (server.isRunning())
				std::this_thread::sleep_for(std::chrono::seconds(1));

			server.Close();
		}
	}
	catch (TCLAP::ArgException &e)
	{
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}

#ifdef SYSTEM_LINUX
	Log_DelCallback(PrintDaemonLog);
	closelog();
#endif

	return 0;
}

