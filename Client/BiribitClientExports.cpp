#include "BiribitClientExports.h"
#include "PrintLog.h"

void BiribitClientClean()
{
	Log_Destroy();
}

int BiribitClientAddLogCallback(BiribitClientLogCallback _pCallback)
{
	Log_Init();
	return Log_AddCallback((LogCallback) _pCallback);
}

//---------------------------------------------------------------------------//

int BiribitClientDelLogCallback(BiribitClientLogCallback _pCallback)
{
	return Log_DelCallback((LogCallback) _pCallback);
}

//---------------------------------------------------------------------------//

void BiribitClientSetLogFile(const char* path)
{
	Log_Init();
	Log_SetLogFile(path);
}

//---------------------------------------------------------------------------//

void BiribitClient_printLog(const char *fmt, ...)
{
	va_list ap;
	va_start (ap, fmt);
	vprintLog(fmt, ap);
	va_end(ap);
}

//---------------------------------------------------------------------------//

void BiribitClient_vprintLog(const char *fmt, va_list ap)
{
	vprintLog(fmt, ap);
}
