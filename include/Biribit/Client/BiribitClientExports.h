#pragma once

#include <Biribit/BiribitConfig.h>
#include <cstdarg>

typedef void (STDCALL *BiribitClientLogCallback)(const char*);

API_C_EXPORT void BiribitClientClean();

////////////////////////////////////////////////////////////
// \fn	int BiribitClientAddLogCallback(BiribitClientLogCallback _pCallback);
//
// \brief	Adds a callback for being call every time the library notifies.
//
// \return	Returns 0 if it already exists, otherwise returns 1.
//
////////////////////////////////////////////////////////////
API_C_EXPORT int BiribitClientAddLogCallback(BiribitClientLogCallback _pCallback);


////////////////////////////////////////////////////////////
// \fn	int BiribitClientDelLogCallback(BiribitClientLogCallback _pCallback);
//
// \brief	Deletes the callback from the list if it's in.
//
// \return	Returns 0 if it doesn't exist, otherwise returns 1.
//
////////////////////////////////////////////////////////////
API_C_EXPORT int BiribitClientDelLogCallback(BiribitClientLogCallback _pCallback);


////////////////////////////////////////////////////////////
// \fn	void BiribitClient_etLogFile(const char* path);
//
// \brief	Adds a build-in callback that stores the log in a file.
//
////////////////////////////////////////////////////////////
API_C_EXPORT void BiribitClientSetLogFile(const char* path);


////////////////////////////////////////////////////////////
// \fn	void BiribitClient_printLog(const char *fmt, ...);
//
// \brief	Prints in the internal log.
//
////////////////////////////////////////////////////////////
API_C_EXPORT void BiribitClient_printLog(const char *fmt, ...);

////////////////////////////////////////////////////////////
// \fn	void BiribitClient_vprintLog(const char *fmt, va_list ap);
//
// \brief	Prints in the internal log.
//
////////////////////////////////////////////////////////////
API_C_EXPORT void BiribitClient_vprintLog(const char *fmt, va_list ap);
