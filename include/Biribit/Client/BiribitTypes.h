#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>

#include <Biribit/BiribitConfig.h>
#include <Biribit/Packet.h>

namespace Biribit
{

typedef std::uint32_t TimeMS;

struct API_EXPORT ServerInfo
{
	std::string name;
	std::string addr;
	std::uint32_t ping;
	unsigned short port;
	bool passwordProtected;

	ServerInfo();
};

struct API_EXPORT ServerConnection
{
	typedef std::uint32_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::string name;
	std::uint32_t ping;

	ServerConnection();
};

struct API_EXPORT RemoteClient
{
	typedef std::uint32_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::string name;
	std::string appid;

	RemoteClient();
};

struct API_EXPORT ClientParameters
{
	std::string name;
	std::string appid;

	ClientParameters();
	ClientParameters(const std::string& name, const std::string& appid);
};

struct API_EXPORT Room
{
	typedef std::uint32_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::vector<RemoteClient::id_t> slots;

	Room();
};

struct API_EXPORT Received
{
	TimeMS when;
	ServerConnection::id_t connection;
	Room::id_t room_id;
	std::uint8_t slot_id;
	Packet data;

	Received();
	Received(Received&&);
};

struct API_EXPORT Entry
{
	typedef std::uint32_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::uint8_t from_slot;
	Packet data;

	Entry();
	Entry(Entry&&);
	~Entry();

	Entry& operator=(Entry&&);
};

} //namespace Biribit