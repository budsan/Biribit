#include "PrintLog.h"
#include "TaskPool.h"

#include <string>
#include <cstdarg>
#include <cstdlib>
#include <stdio.h>
#include <time.h>

#include <set>
#include <mutex>

namespace Log
{
	std::unique_ptr<TaskPool> printPool;

	void CreateThread()
	{
		if (printPool == nullptr) {
			printPool = std::unique_ptr<TaskPool>(new TaskPool(1, "PrintLog")); //std::make_unique<TaskPool>(1, "PrintLog");
		}
	}

	void DestroyThread()
	{
		printPool = nullptr;
	}

	template<class F, class... Args> auto enqueue(F&& f, Args&&... args)
		-> std::future<typename std::result_of<F(Args...)>::type>
	{
		if (printPool != nullptr)
			return printPool->enqueue(std::forward<F>(f), std::forward<Args>(args)...);

		using return_type = typename std::result_of<F(Args...)>::type;
		auto task = std::make_shared< std::packaged_task<return_type()> >(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

		std::future<return_type> res = task->get_future();
		(*task)();
		return res;
	}
}

std::set<LogCallback> &callbacks()
{
	static std::set<LogCallback> *_callbacks = NULL;
	if (_callbacks == NULL) {
		_callbacks = new std::set<LogCallback>();
	}

	return *_callbacks;
}

void Log_Init()
{
	Log::CreateThread();
}

void Log_Destroy()
{
	callbacks().clear();
	Log::DestroyThread();
}

int Log_AddCallback(LogCallback _pCallback)
{
	auto result = Log::enqueue([_pCallback]()-> int {
		auto ret = callbacks().insert(_pCallback);
		return ret.second ? 1 : 0;
	});

	return result.get();
}

int Log_DelCallback(LogCallback _pCallback)
{
	auto result = Log::enqueue([_pCallback]()-> int {
		auto it = callbacks().find(_pCallback);
		if (it != callbacks().end()) {
			callbacks().erase(it);
			return 1;
		}

		return 0;
	});

	return result.get();
}

static FILE* sLogFile = NULL;
void STDCALL printToFile(const char *msg)
{
	if (sLogFile != NULL)
	{
		fprintf(sLogFile, "%s\n", msg);
		fflush(sLogFile);
	}
}

void Log_SetLogFile(const char* path)
{
	if (path == NULL)
	{
		if (sLogFile != NULL)
		{
			Log_DelCallback(printToFile);
			fclose(sLogFile);
			sLogFile = NULL;
		}
	}
	else
	{
		FILE* fLog;
		fLog = fopen(path, "a");
		if (fLog != NULL)
		{
			sLogFile = fLog;

			time_t t;
			time(&t);

			struct tm * ptm;
			ptm = localtime(&t);
			fprintf(sLogFile, "-- Log opened at %2d:%02d:%02d --\n\n", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

			Log_AddCallback(printToFile);
		}
	}
}

void printLog(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	::vprintLog(fmt, ap);
	va_end(ap);
}


void vprintLog(const char *fmt, va_list ap)
{
	if (fmt == NULL) return;
	time_t t;
	time(&t);

	struct tm * ptm;
	ptm = localtime(&t);

	char time[64];
	sprintf(time, "[%2d:%02d:%02d] ", ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

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

	std::string res = std::string(time) + str;
	Log::enqueue([res]()
	{
		if (callbacks().empty())
			printf("%s\n", res.c_str());
		else
			for (auto it = callbacks().begin(); it != callbacks().end(); ++it)
				(*it)(res.c_str());
	});
}
