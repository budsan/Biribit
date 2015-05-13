#pragma once

#include "MessageIdentifiers.h"
#include <ServerInfo.pb.h>

enum BiribittMessageIDTypes
{
	ID_SERVER_INFO_REQUEST = ID_USER_PACKET_ENUM, //nothing follows
	ID_SERVER_INFO_RESPONSE, //follows Message(ServerInfo)
	ID_CLIENT_NAME_SET, // follows string(name)
	ID_CLIENT_NAME_IN_USE, //nothing follows
	ID_CLIENT_NAME_SET_SUCCESS, //follows string(name)
	ID_SERVER_CLIENT_NAME_CHANGED // follows string(name)
};
