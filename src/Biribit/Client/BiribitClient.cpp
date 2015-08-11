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

void Client::Disconnect(Connection::id_t id)
{
	m_impl->Disconnect(id);
}

void Client::Disconnect()
{
	m_impl->Disconnect();
}

void Client::DiscoverServersOnLAN(unsigned short port)
{
	m_impl->DiscoverServersOnLAN(port);
}

void Client::ClearServerList()
{
	m_impl->ClearServerList();
}

void Client::RefreshServerList()
{
	m_impl->RefreshServerList();
}

std::future<std::vector<ServerInfo>> Client::GetFutureServerList()
{
	return m_impl->GetFutureServerList();
}

std::future<std::vector<Connection>> Client::GetFutureConnections()
{
	return m_impl->GetFutureConnections();
}

std::future<std::vector<RemoteClient>> Client::GetFutureRemoteClients(Connection::id_t id)
{
	return m_impl->GetFutureRemoteClients(id);
}

future_vector<Room> Client::GetFutureRooms(Connection::id_t id)
{
	return m_impl->GetFutureRooms(id);
}

/*
const std::vector<ServerInfo>& Client::GetServerList(std::uint32_t* revision)
{
	return m_impl->GetServerList(revision);
}

const std::vector<Connection>& Client::GetConnections(std::uint32_t* revision)
{
	return m_impl->GetConnections(revision);
}

const std::vector<RemoteClient>& Client::GetRemoteClients(Connection::id_t id, std::uint32_t* revision)
{
	return m_impl->GetRemoteClients(id, revision);
}

const std::vector<Room>& Client::GetRooms(Connection::id_t id, std::uint32_t* revision)
{
return m_impl->GetRooms(id, revision);
}

*/

RemoteClient::id_t Client::GetLocalClientId(Connection::id_t id)
{
	return m_impl->GetLocalClientId(id);
}

void Client::SetLocalClientParameters(Connection::id_t id, const ClientParameters& parameters)
{
	m_impl->SetLocalClientParameters(id, parameters);
}

void Client::RefreshRooms(Connection::id_t id)
{
	m_impl->RefreshRooms(id);
}

void Client::CreateRoom(Connection::id_t id, Room::slot_id_t num_slots)
{
	m_impl->CreateRoom(id, num_slots);
}

void Client::CreateRoom(Connection::id_t id, Room::slot_id_t num_slots, Room::slot_id_t slot_to_join_id)
{
	m_impl->CreateRoom(id, num_slots, slot_to_join_id);
}

void Client::JoinRandomOrCreateRoom(Connection::id_t id, Room::slot_id_t num_slots)
{
	m_impl->JoinRandomOrCreateRoom(id, num_slots);
}

void Client::JoinRoom(Connection::id_t id, Room::id_t room_id)
{
	m_impl->JoinRoom(id, room_id);
}

void Client::JoinRoom(Connection::id_t id, Room::id_t room_id, Room::slot_id_t slot_id)
{
	m_impl->JoinRoom(id, room_id, slot_id);
}

Room::id_t Client::GetJoinedRoomId(Connection::id_t id)
{
	return m_impl->GetJoinedRoomId(id);
}

Room::slot_id_t Client::GetJoinedRoomSlot(Connection::id_t id)
{
	return m_impl->GetJoinedRoomSlot(id);
}

void Client::SendBroadcast(Connection::id_t id, const Packet& packet, Packet::ReliabilityBitmask mask)
{
	m_impl->SendBroadcast(id, packet, mask);
}

void Client::SendBroadcast(Connection::id_t id, const char* data, unsigned int lenght, Packet::ReliabilityBitmask mask)
{
	m_impl->SendBroadcast(id, data, lenght, mask);
}

void Client::SendEntry(Connection::id_t id, const Packet& packet)
{
	m_impl->SendEntry(id, packet);
}

void Client::SendEntry(Connection::id_t id, const char* data, unsigned int lenght)
{
	m_impl->SendEntry(id, data, lenght);
}

std::unique_ptr<Event> Client::PullEvent()
{
	return m_impl->PullEvent();
}

Entry::id_t Client::GetEntriesCount(Connection::id_t id)
{
	return m_impl->GetEntriesCount(id);
}

const Entry& Client::GetEntry(Connection::id_t id, Entry::id_t entryId)
{
	return m_impl->GetEntry(id, entryId);
}

}