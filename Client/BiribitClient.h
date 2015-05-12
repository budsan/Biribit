#pragma once

#include <string>
#include <cstdint>
#include <vector>

#include "Config.h"

struct API_EXPORT ServerDescription
{
		std::string name;
		std::string addr;
		std::uint32_t ping;
		unsigned short port;
		bool passwordProtected;

		ServerDescription();
};

struct API_EXPORT ServerConnection
{
	typedef std::size_t id_t;
	enum { UNASSIGNED_ID = 0 };

	id_t id;
	std::string name;

	ServerConnection();
};

class BiribitClientImpl;
class API_EXPORT BiribitClient
{
public:

	BiribitClient();
	virtual ~BiribitClient();

	void Connect(const char* addr = nullptr, unsigned short port = 0, const char* password = nullptr);
	void Disconnect(ServerConnection::id_t id);
	void Disconnect();

	void DiscoverOnLan(unsigned short port = 0);
	void ClearDiscoverInfo();
	void RefreshDiscoverInfo();
	const std::vector<ServerDescription>& GetDiscoverInfo();
	const std::vector<ServerConnection>& GetConnections();

private:
	BiribitClientImpl* m_impl;
};
