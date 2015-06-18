#pragma once

#include <Biribit/Client/BiribitTypes.h>

namespace Biribit
{

struct ServerInfoPriv
{
	ServerInfo data;
	ServerConnection::id_t id;
	bool valid;

	ServerInfoPriv();
};

} // namespace Biribit