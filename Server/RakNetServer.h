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

	unique<TaskPool> m_pool;
	Generic::TempBuffer m_buffer;

	static void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
	void RakNetUpdated();
	void HandlePacket(RakNet::Packet*);

	bool WriteMessage(RakNet::BitStream& bstream,
		RakNet::MessageID msgId,
		::google::protobuf::MessageLite& msg);

	template<typename T> bool ReadMessage(T& msg, RakNet::BitStream& bstream);

public:

	RakNetServer();

	bool Run(unsigned short port = 0, const char* name = NULL, const char* password = NULL);
	bool isRunning();
	bool Close();
};
