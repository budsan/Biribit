#pragma once

#include <ServerInfo.pb.h>
#include <Client.pb.h>
#include <Room.pb.h>
#include <ServerStatus.pb.h>

#include "MessageIdentifiers.h"

enum BiribitMessageIDTypes
{
	ID_SERVER_INFO_REQUEST = ID_USER_PACKET_ENUM,
	//cl > sv: nothing follows

	ID_SERVER_INFO_RESPONSE,
	//sv > cl: follows Proto::ServerInfo

	ID_SERVER_STATUS_REQUEST,
	//cl > sv: nothing follows

	ID_SERVER_STATUS_RESPONSE,
	//sv > cl: follows Proto::ServerStatus

	ID_CLIENT_UPDATE_STATUS,
	//cl > sv: follows Proto::ClientUpdate

	ID_CLIENT_NAME_IN_USE,
	//sv > cl: nothing follows

	ID_CLIENT_STATUS_UPDATED,
	//sv > cl: follows Proto::Client

	ID_CLIENT_DISCONNECTED
	//sv > cl: follows Proto::Client
};

namespace Biribit
{
	struct Client
	{
		typedef std::size_t id_t;
		enum { UNASSIGNED_ID = 0 };

		std::size_t id;
		std::string name;
		std::string appid;
	};
}
