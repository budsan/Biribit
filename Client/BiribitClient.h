#pragma once

#include "Config.h"

#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>

#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakNetStatistics.h"
#include "RakNetTypes.h"
#include "BitStream.h"
#include "PacketLogger.h"
#include "RakNetTypes.h"
#include "StringCompressor.h"
#include "GetTime.h"

#include "RefSwap.h"
#include "TaskPool.h"
#include "Types.h"

class API_EXPORT ServerInfo
{
public:
		std::string name;
		std::uint32_t ping;
		RakNet::SystemAddress addr;
		bool valid;

		ServerInfo();
};

class API_EXPORT BiribitClient
{
public:

	BiribitClient();
	~BiribitClient();

	void Connect(const char* addr = nullptr, unsigned short port = 0, const char* password = nullptr);
	void Disconnect();

	void DiscoverOnLan(unsigned short port = 0);
	void ClearDiscoverInfo();
	void RefreshDiscoverInfo();
	const std::vector<ServerInfo>& GetDiscoverInfo();

private:
	RakNet::RakPeerInterface *m_peer;
	unique<TaskPool> m_pool;

	static void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
	void RakNetUpdated();
	void HandlePacket(RakNet::Packet*);

	std::map<RakNet::SystemAddress, ServerInfo> serverList;
	RefSwap<std::vector<ServerInfo>> serverListReq;
	bool serverListReqReady;
};
