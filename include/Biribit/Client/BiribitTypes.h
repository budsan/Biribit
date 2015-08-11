#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <memory>
#include <future>

#include <Biribit/BiribitConfig.h>
#include <Biribit/Packet.h>

namespace Biribit
{

using milliseconds_t = std::uint32_t ;
using id_t = std::uint32_t;
enum { UNASSIGNED_ID = 0 };
template<class T> using future_vector = std::future<std::vector<T>>;

struct API_EXPORT ServerInfo
{
	std::string name;
	std::string addr;
	std::uint32_t ping;
	unsigned short port;
	bool passwordProtected;
	unsigned int max_clients;
	unsigned int connected_clients;

	ServerInfo();
};

struct API_EXPORT Connection
{
	using id_t = Biribit::id_t;
	enum { UNASSIGNED_ID = Biribit::UNASSIGNED_ID};

	id_t id;
	std::string name;
	std::uint32_t ping;

	Connection();
};

struct API_EXPORT RemoteClient
{
	using id_t = Biribit::id_t;
	enum { UNASSIGNED_ID = Biribit::UNASSIGNED_ID };

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
	using id_t = Biribit::id_t;
	using slot_id_t = std::uint8_t;
	enum { UNASSIGNED_ID = Biribit::UNASSIGNED_ID };

	id_t id;
	std::vector<RemoteClient::id_t> slots;

	Room();
};

struct API_EXPORT Received
{
	milliseconds_t when;
	Connection::id_t connection;
	Room::id_t room_id;
	Room::slot_id_t slot_id;
	Packet data;

	Received();
	Received(Received&&);
};

struct API_EXPORT Entry
{
	using id_t = Biribit::id_t;
	enum { UNASSIGNED_ID = Biribit::UNASSIGNED_ID };

	id_t id;
	std::uint8_t from_slot;
	Packet data;

	Entry();
	Entry(Entry&&);
	~Entry();

	Entry& operator=(Entry&&);
};

} //namespace Biribit