#pragma once

#include <ServerInfo.pb.h>
#include <Client.pb.h>
#include <Room.pb.h>
#include <ServerStatus.pb.h>

//RakNet
#include <MessageIdentifiers.h>

const unsigned short SERVER_DEFAULT_PORT = 8287;
const unsigned int   SERVER_MAX_NUM_CLIENTS = 42;
const unsigned int   CLIENT_MAX_CONNECTIONS = 8;

enum BiribitMessageIDTypes
{
	ID_SERVER_INFO_REQUEST = ID_USER_PACKET_ENUM,
	//cl > sv: nothing follows

	ID_SERVER_INFO_RESPONSE,
	//sv > cl: follows Proto::ServerInfo

	ID_CLIENT_SELF_STATUS,
	//sv > cl: follows Proto::Client

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

	ID_CLIENT_DISCONNECTED,
	//sv > cl: follows Proto::Client

	ID_ROOM_LIST_REQUEST,
	//cl > sv: nothing follows

	ID_ROOM_LIST_RESPONSE,
	//sv > cl: follows Proto::RoomList

	ID_ROOM_CREATE_REQUEST,
	//cl > sv: follows Proto::RoomCreateRequest

	ID_ROOM_CREATE_RESPONSE,
	//sv -> cl: follows Proto:Room

	ID_ROOM_JOIN_REQUEST,
	//cl -> sv: follows Proto::RoomJoin

	ID_ROOM_JOIN_RESPONSE
	//sv -> cl: follows Proto::RoomJoin
};
