#pragma once

#include <string>
#include <cstdint>
#include <vector>

#include "Config.h"

struct API_EXPORT ServerInfo
{
		std::string name;
		std::string addr;
		std::uint32_t ping;
		unsigned short port;
		bool valid;

		ServerInfo();
};

class BiribitClientImpl;
class API_EXPORT BiribitClient
{
public:

	BiribitClient();
	virtual ~BiribitClient();

	void Connect(const char* addr = nullptr, unsigned short port = 0, const char* password = nullptr);
	void Disconnect();

	void DiscoverOnLan(unsigned short port = 0);
	void ClearDiscoverInfo();
	void RefreshDiscoverInfo();
	const std::vector<ServerInfo>& GetDiscoverInfo();

private:
	BiribitClientImpl* m_impl;
};
