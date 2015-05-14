#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>

#include "RakPeerInterface.h"
#include "TaskPool.h"
#include <Types.h>
#include <Generic.h>

#include <BiribitMessageIdentifiers.h>

class RakNetServer
{
	RakNet::RakPeerInterface *m_peer;
	std::string m_name;
	bool m_passwordProtected;

	struct Client
	{
		typedef std::size_t id_t;
		enum { UNASSIGNED_ID = 0 };

		id_t id;
		std::string name;
		std::string appid;
		RakNet::SystemAddress addr;
	};

	std::vector<unique<Client>> m_clients;
	std::map<RakNet::SystemAddress, std::size_t> m_clientAddrMap;
	std::map<std::string, std::size_t> m_clientNameMap;

	Client::id_t NewClient(RakNet::SystemAddress addr);
	void RemoveClient(RakNet::SystemAddress addr);
	void UpdateClient(RakNet::SystemAddress addr, Proto::ClientUpdate* proto_update);

	void PopulateProtoServerInfo(Proto::ServerInfo* proto_info);
	void PopulateProtoClient(unique<Client>& client, Proto::Client* proto_client);

	unique<TaskPool> m_pool;
	Generic::TempBuffer m_buffer;

	static void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
	void RakNetUpdated();
	void HandlePacket(RakNet::Packet*);

	bool WriteMessage(RakNet::BitStream& bstream, RakNet::MessageID msgId, ::google::protobuf::MessageLite& msg);
	template<typename T> bool ReadMessage(T& msg, RakNet::BitStream& bstream);

public:

	RakNetServer();

	bool Run(unsigned short port = 0, const char* name = NULL, const char* password = NULL);
	bool isRunning();
	bool Close();
};
