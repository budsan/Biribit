#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>

#include "RakPeerInterface.h"
#include "ServerListener.h"
#include "TaskPool.h"

class RakNetServer : public Server
{
	RakNet::RakPeerInterface *m_peer;
	std::string m_name;

	unique<TaskPool> m_pool;
	shared<ServerListener> m_listener;

	static void RaknetThreadUpdate(RakNet::RakPeerInterface *peer, void* data);
	void RakNetUpdated();
	void HandlePacket(RakNet::Packet*);

public:

	RakNetServer();

	bool Run(shared<ServerListener> _listener, unsigned short port = 0, const char* name = NULL, const char* password = NULL);
	bool isRunning();
	bool Close();

	void Send(std::uint64_t id, shared<Packet> packet) override;
	void AdvertiseSystem(const std::string &addr, unsigned short port, shared<Packet> packet) override;
};
