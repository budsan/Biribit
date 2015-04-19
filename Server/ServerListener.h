#pragma once

#include <cstdint>
#include "Packet.h"
#include "Types.h"
#include "TaskPool.h"

class Server
{
public:
	virtual void Send(std::uint64_t id, shared<Packet> packet) = 0;
	virtual void AdvertiseSystem(const std::string &addr, unsigned short port, shared<Packet> packet) = 0;
};

class ServerListener
{
public:
	virtual void OnDisconnectionNotification(Server& server, std::uint64_t id) {}
	virtual void OnNewIncomingConnection(Server& server, std::uint64_t id) {}
	virtual void OnAdvertisement(Server& server, const std::string &addr, unsigned short port, shared<Packet> packet) {}
	virtual void OnConnectionLost(Server& server, std::uint64_t id) {}
	virtual void OnPacket(Server& server, shared<Packet> packet) {}
};
