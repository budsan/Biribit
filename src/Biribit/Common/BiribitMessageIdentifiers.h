#pragma once

#include <ServerInfo.pb.h>
#include <Client.pb.h>
#include <Room.pb.h>
#include <ServerStatus.pb.h>

//RakNet
#include <MessageIdentifiers.h>

const unsigned short SERVER_DEFAULT_PORT = 8287;
const unsigned int   SERVER_DEFAULT_MAX_CONNECTIONS = 32;
const unsigned int   CLIENT_MAX_CONNECTIONS = 8;

enum BiribitMessageIDTypes
{
	ID_ERROR_CODE = ID_USER_PACKET_ENUM,
	//sv > cl: follows BiribitErrorTypes(uin32_t)

	ID_SERVER_INFO_REQUEST,
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

	ID_CLIENT_STATUS_UPDATED,
	//sv > cl: follows Proto::Client

	ID_CLIENT_DISCONNECTED,
	//sv > cl: follows Proto::Client

	ID_ROOM_LIST_REQUEST,
	//cl > sv: nothing follows

	ID_ROOM_LIST_RESPONSE,
	//sv > cl: follows Proto::RoomList

	ID_ROOM_CREATE_REQUEST,
	//cl > sv: follows Proto::RoomCreate

	ID_ROOM_JOIN_RANDOM_OR_CREATE_REQUEST,
	//cl -> sv: follows Proto::RoomCreate

	ID_ROOM_JOIN_REQUEST,
	//cl -> sv: follows Proto::RoomJoin

	ID_ROOM_STATUS,
	//sv -> cl: follows Proto:Room

	ID_ROOM_JOIN_RESPONSE,
	//sv -> cl: follows Proto::RoomJoin

	ID_SEND_BROADCAST_TO_ROOM,
	//cl -> sv: follows reliability(uin8_t) + binary data

	ID_BROADCAST_FROM_ROOM,
	//sv -> cl: follows sender_slot(uin8_t) + binary data

	ID_JOURNAL_ENTRIES_REQUEST,
	//cl -> sv: follows Proto::RoomEntriesRequest

	ID_JOURNAL_ENTRIES_STATUS,
	//sv -> cl: follows Proto::RoomEntriesStatus

	ID_SEND_ENTRY_TO_ROOM
	//cl -> sv: follows binary data
};


