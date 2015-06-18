#include <Biribit/Client/BiribitTypes.h>
#include <RakNetTypes.h>

namespace Biribit
{

ServerInfo::ServerInfo()
	: name()
	, addr(RakNet::UNASSIGNED_SYSTEM_ADDRESS.ToString(false))
	, ping(0)
	, port(0)
	, passwordProtected(false)
{
}

//---------------------------------------------------------------------------//

Connection::Connection() : id(Connection::UNASSIGNED_ID)
{
}

//---------------------------------------------------------------------------//

RemoteClient::RemoteClient() : id(RemoteClient::UNASSIGNED_ID)
{
}

//---------------------------------------------------------------------------//

ClientParameters::ClientParameters()
{
}

ClientParameters::ClientParameters(const std::string& name, const std::string& appid)
	: name(name)
	, appid(appid)
{
}

//---------------------------------------------------------------------------//

Room::Room()
	: id(Room::UNASSIGNED_ID)
{
}

//---------------------------------------------------------------------------//

Received::Received()
	: when(0)
	, connection(Connection::UNASSIGNED_ID)
	, room_id(Room::UNASSIGNED_ID)
	, slot_id(0)
	, data()
{
}

Received::Received(Received&& to_move)
{
	std::swap(when, to_move.when);
	std::swap(connection, to_move.connection);
	std::swap(room_id, to_move.room_id);
	std::swap(slot_id, to_move.slot_id);
	data = std::move(to_move.data);
}

//---------------------------------------------------------------------------//

Entry::Entry()
	: id(UNASSIGNED_ID)
	, from_slot(0)
	, data()
{
}

Entry::Entry(Entry&& to_move)
{
	this->operator=(std::move(to_move));
}

Entry::~Entry()
{
}

Entry& Entry::operator=(Entry&& to_move)
{
	std::swap(id, to_move.id);
	std::swap(from_slot, to_move.from_slot);
	data = std::move(data);
	return *this;
}

} // namespace Biribit