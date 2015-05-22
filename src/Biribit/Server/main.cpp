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
// http://www.4pmp.com/2009/12/a-simple-daemon-in-c/

#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DAEMON_NAME "BiribitServer"

void daemonShutdown();
void signal_handler(int sig);
void daemonize(char *rundir, char *pidfile);

int pidFilehandle;
void signal_handler(int sig)
{
	switch (sig)
	{
	case SIGHUP:
		syslog(LOG_WARNING, "Received SIGHUP signal.");
		break;
	case SIGINT:
	case SIGTERM:
		server.Close();
		break;
	default:
		syslog(LOG_WARNING, "Unhandled signal %s", strsignal(sig));
		break;
	}
}

void daemonShutdown()
{
	Log_DelCallback(PrintDaemonLog);
	close(pidFilehandle);
	closelog();
}

void STDCALL PrintDaemonLog(const char* msg)
{
	syslog(LOG_NOTICE, msg);
}

void daemonize(const char *rundir, const char *pidfile)
{
	int pid, sid, i;
	char str[10];
	struct sigaction newSigAction;
	sigset_t newSigSet;

	/* Check if parent process id is set */
	if (getppid() == 1)
	{
		/* PPID exists, therefore we are already a daemon */
		return;
	}

	/* Set signal mask - signals we want to block */
	sigemptyset(&newSigSet);
	sigaddset(&newSigSet, SIGCHLD);  /* ignore child - i.e. we don't need to wait for it */
	sigaddset(&newSigSet, SIGTSTP);  /* ignore Tty stop signals */
	sigaddset(&newSigSet, SIGTTOU);  /* ignore Tty background writes */
	sigaddset(&newSigSet, SIGTTIN);  /* ignore Tty background reads */
	sigprocmask(SIG_BLOCK, &newSigSet, NULL);   /* Block the above specified signals */

	/* Set up a signal handler */
	newSigAction.sa_handler = signal_handler;
	sigemptyset(&newSigAction.sa_mask);
	newSigAction.sa_flags = 0;

	/* Signals to handle */
	sigaction(SIGHUP, &newSigAction, NULL);     /* catch hangup signal */
	sigaction(SIGTERM, &newSigAction, NULL);    /* catch term signal */
	sigaction(SIGINT, &newSigAction, NULL);     /* catch interrupt signal */


	/* Fork*/
	pid = fork();

	if (pid < 0)
	{
		/* Could not fork */
		exit(EXIT_FAILURE);
	}

	if (pid > 0)
	{
		/* Child created ok, so exit parent process */
		printf("Child process created: %d\n", pid);
		exit(EXIT_SUCCESS);
	}

	/* Child continues */

	umask(027); /* Set file permissions 750 */

	/* Get a new process group */
	sid = setsid();

	if (sid < 0)
	{
		exit(EXIT_FAILURE);
	}

	/* close all descriptors */
	for (i = getdtablesize(); i >= 0; --i)
	{
		close(i);
	}

	/* Route I/O connections */
	openlog(DAEMON_NAME, LOG_PID, LOG_DAEMON);

	/* Open STDIN */
	i = open("/dev/null", O_RDWR);

	/* STDOUT */
	dup(i);

	/* STDERR */
	dup(i);

	chdir(rundir); /* change running directory */

	/* Ensure only one copy */
	pidFilehandle = open(pidfile, O_RDWR | O_CREAT, 0600);

	if (pidFilehandle == -1)
	{
		/* Couldn't open lock file */
		syslog(LOG_INFO, "Could not open PID lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}

	/* Try to lock file */
	if (lockf(pidFilehandle, F_TLOCK, 0) == -1)
	{
		/* Couldn't get lock on lock file */
		syslog(LOG_INFO, "Could not lock PID lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}


	/* Get and format PID */
	sprintf(str, "%d\n", getpid());

	/* write pid to lockfile */
	write(pidFilehandle, str, strlen(str));
}

class Daemon
{
public:
	Daemon(const char* rundir, const char* pidfile)
	{
		daemonize(rundir, pidfile);
	}

	~Daemon()
	{
		daemonShutdown();
	}
};

#endif

int main(int argc, char** argv)
{
#ifdef SYSTEM_WINDOWS
	BOOL ret = SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);
#endif

	try
	{
		TCLAP::CmdLine cmd("Biribit server", ' ', "0.1");

		TCLAP::ValueArg<std::string> nameArg0("n", "name", "Server name shown", false, "", "name");
		cmd.add(nameArg0);

		TCLAP::ValueArg<std::string> nameArg1("p", "port", "Port to bind", false, "", "port");
		cmd.add(nameArg1);

		TCLAP::ValueArg<std::string> nameArg2("w", "password", "Server password", false, "", "password");
		cmd.add(nameArg2);

#ifdef SYSTEM_LINUX
		TCLAP::ValueArg<std::string> nameArgPID("i", "pidfile", "PID File", false, "", "pid");
		cmd.add(nameArgPID);
#endif
		
		cmd.parse(argc, argv);

		// Get the value parsed by each arg. 
		std::string name = nameArg0.getValue();
		std::string port = nameArg1.getValue();
		std::string pass = nameArg2.getValue();

#ifdef SYSTEM_LINUX
		std::string pidfile = nameArgPID.getValue();
		std::unique_ptr<Daemon> daemon;
		if (!pidfile.empty())
			daemon = std::unique_ptr<Daemon>(new Daemon("/tmp/", pidfile.c_str()));
#endif

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

	return 0;
}

