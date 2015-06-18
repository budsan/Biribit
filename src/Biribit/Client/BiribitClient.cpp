#include <Biribit/Client/BiribitClient.h>
#include "BiribitClientImpl.h"

namespace Biribit
{

Client::Client() : m_impl(new ClientImpl())
{
}

Client::~Client()
{
	delete m_impl;
}

void Client::Connect(const char* addr, unsigned short port, const char* password)
{
	m_impl->Connect(addr, port, password);
}

void Client::Disconnect(ServerConnection::id_t id)
{
	m_impl->Disconnect(id);
}

void Client::Disconnect()
{
	m_impl->Disconnect();
}

void Client::DiscoverOnLan(unsigned short port)
{
	m_impl->DiscoverOnLan(port);
}

void Client::ClearDiscoverInfo()
{
	m_impl->ClearDiscoverInfo();
}

void Client::RefreshDiscoverInfo()
{
	m_impl->RefreshDiscoverInfo();
}

const std::vector<ServerInfo>& Client::GetDiscoverInfo(std::uint32_t* revision)
{
	return m_impl->GetDiscoverInfo(revision);
}

const std::vector<ServerConnection>& Client::GetConnections(std::uint32_t* revision)
{
	return m_impl->GetConnections(revision);
}

const std::vector<RemoteClient>& Client::GetRemoteClients(ServerConnection::id_t id, std::uint32_t* revision)
{
	return m_impl->GetRemoteClients(id, revision);
}

RemoteClient::id_t Client::GetLocalClientId(ServerConnection::id_t id)
{
	return m_impl->GetLocalClientId(id);
}

void Client::SetLocalClientParameters(ServerConnection::id_t id, const ClientParameters& parameters)
{
	m_impl->SetLocalClientParameters(id, parameters);
}

void Client::RefreshRooms(ServerConnection::id_t id)
{
	m_impl->RefreshRooms(id);
}

const std::vector<Room>& Client::GetRooms(ServerConnection::id_t id, std::uint32_t* revision)
{
	return m_impl->GetRooms(id, revision);
}

void Client::CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots)
{
	m_impl->CreateRoom(id, num_slots);
}

void Client::CreateRoom(ServerConnection::id_t id, std::uint32_t num_slots, std::uint32_t slot_to_join_id)
{
	m_impl->CreateRoom(id, num_slots, slot_to_join_id);
}

void Client::JoinRandomOrCreateRoom(ServerConnection::id_t id, std::uint32_t num_slots)
{
	m_impl->JoinRandomOrCreateRoom(id, num_slots);
}

void Client::JoinRoom(ServerConnection::id_t id, Room::id_t room_id)
{
	m_impl->JoinRoom(id, room_id);
}

void Client::JoinRoom(ServerConnection::id_t id, Room::id_t room_id, std::uint32_t slot_id)
{
	m_impl->JoinRoom(id, room_id, slot_id);
}

Room::id_t Client::GetJoinedRoomId(ServerConnection::id_t id)
{
	return m_impl->GetJoinedRoomId(id);
}

std::uint32_t Client::GetJoinedRoomSlot(ServerConnection::id_t id)
{
	return m_impl->GetJoinedRoomSlot(id);
}

void Client::SendBroadcast(ServerConnection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask)
{
	m_impl->SendBroadcast(id, packet, mask);
}

void Client::SendBroadcast(ServerConnection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask)
{
	m_impl->SendBroadcast(id, data, lenght, mask);
}

void Client::SendEntry(ServerConnection::id_t id, const Packet& packet)
{
	m_impl->SendEntry(id, packet);
}

void Client::SendEntry(ServerConnection::id_t id, const char* data, unsigned int lenght)
{
	m_impl->SendEntry(id, data, lenght);
}

std::size_t Client::GetDataSizeOfNextReceived()
{
	return m_impl->GetDataSizeOfNextReceived();
}

std::unique_ptr<Received> Client::PullReceived()
{
	return m_impl->PullReceived();
}

Entry::id_t Client::GetEntriesCount(ServerConnection::id_t id)
{
	return m_impl->GetEntriesCount(id);
}

const Entry& Client::GetEntry(ServerConnection::id_t id, Entry::id_t entryId)
{
	return m_impl->GetEntry(id, entryId);
}

}