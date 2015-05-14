#pragma once

#include <string>
#include <cstdint>
#include <vector>

#include "Config.h"

namespace Biribit
{

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
	typedef std::size_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::string name;
	std::uint32_t ping;

	ServerConnection();
};

struct API_EXPORT RemoteClient
{
	typedef std::size_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::string name;
	std::string appid;
};

class ClientImpl;
class API_EXPORT Client
{
public:

	Client();
	virtual ~Client();

	void Connect(const char* addr = nullptr, unsigned short port = 0, const char* password = nullptr);
	void Disconnect(ServerConnection::id_t id);
	void Disconnect();

	void DiscoverOnLan(unsigned short port = 0);
	void ClearDiscoverInfo();
	void RefreshDiscoverInfo();

	const std::vector<ServerInfo>& GetDiscoverInfo();
	const std::vector<ServerConnection>& GetConnections();

private:
	ClientImpl* m_impl;
};

}