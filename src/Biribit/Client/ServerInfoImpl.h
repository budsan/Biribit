#pragma once

#include <Biribit/Client/BiribitTypes.h>

namespace Biribit
{

struct ServerInfoImpl
{
	ServerInfo data;
	Connection::id_t id;
	bool valid;

	ServerInfoImpl();
};

} // namespace Biribit