#pragma once

#include <BiribitConfig.h>
#include <stdarg.h>

void Log_Init();
void Log_Destroy();

typedef void (STDCALL *LogCallback)(const char*);
int Log_AddCallback(LogCallback _pCallback);
int Log_DelCallback(LogCallback _pCallback);
void Log_SetLogFile(const char* path);

void printLog(const char *fmt, ...);
void vprintLog(const char *fmt, va_list ap);