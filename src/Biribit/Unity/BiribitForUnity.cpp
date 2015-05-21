#include <Biribit/Unity/BiribitForUnity.h>
#include <Biribit/Common/PrintLog.h>
#include <Biribit/Client.h>

#include <memory>
#include <cstdarg>
#include <chrono>

namespace priv
{
	std::shared_ptr<Biribit::Client> client;
}

std::shared_ptr<Biribit::Client>& BiribitForUnity_GetClient()
{
	if (priv::client == nullptr)
		priv::client = std::shared_ptr<Biribit::Client>(new Biribit::Client());

	return priv::client;
}

enum ConnectingState
{
	Connecting_Nothing,
	Connecting_Start,
	Connecting_FindingInLAN,
	Connecting_Refreshing,
	Connecting_Connecting,
	Connecting_Disconnecting
};


ConnectingState c_state = Connecting_Nothing;
std::chrono::system_clock::time_point c_timestamp;
std::string clientname;
std::string appid;
bool connected = false;

void BiribitForUnity_Connect(const char* _clientname, const char* _appid)
{
	clientname = _clientname;
	appid = _appid;

	if (c_state == Connecting_Nothing)
		c_state = Connecting_Start;
}

void BiribitForUnity_Disconnect()
{
	c_state = Connecting_Disconnecting;
}

int BiribitForUnity_IsConnected()
{
	return connected ? 1 : 0;
}

enum JoiningState
{
	Joining_Nothing,
	Joining_Start,
	Joining_Refreshing,
	Joining_Creating,
	Joining_Joining,
	Joining_Leaving
};

JoiningState j_state = Joining_Nothing;
std::chrono::system_clock::time_point j_timestamp;
int num_slots = 0;
int slot_to_join_id = -1;

int joined_room_id = 0;
int joined_room_slot = 0;

void BiribitForUnity_JoinOrCreateRoom(int _num_slots, int _slot_to_join_id)
{
	num_slots = _num_slots;
	slot_to_join_id = _slot_to_join_id;

	if (j_state == Joining_Nothing)
		j_state = Joining_Start;
}

int BiribitForUnity_JoinedRoomId()
{
	return joined_room_id;
}

int BiribitForUnity_JoinedRoomSlot()
{
	return joined_room_slot;
}

void BiribitForUnity_SendToRoom(const char* data, int length, int reliability)
{

}

void BiribitForUnity_PullReceived(const char* data, int length, int reliability)
{

}

void BiribitForUnity_Update()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	std::shared_ptr<Biribit::Client>& client = BiribitForUnity_GetClient();
	const std::vector<Biribit::ServerConnection>& con = client->GetConnections();
	const std::vector<Biribit::ServerInfo>& info = client->GetDiscoverInfo();

	connected = !con.empty();

	//Connect Logic
	if (c_state != Connecting_Nothing) {
		if (connected) {
			if (c_state == Connecting_Connecting) {
				Biribit::ClientParameters params(clientname, appid);
				client->SetLocalClientParameters(con[0].id, params);
			}

			if (c_state == Connecting_Disconnecting) {
				client->Disconnect(con[0].id);
			}
			else {
				c_state = Connecting_Nothing;
			}
		}
		else { // not connected
			if (c_state == Connecting_Start) {
				c_timestamp = now;
				client->DiscoverOnLan();
				c_state = Connecting_FindingInLAN;
			}

			if (c_state == Connecting_FindingInLAN) {
				if ((now - c_timestamp) > std::chrono::seconds(2)) {
					c_timestamp = now;
					client->RefreshDiscoverInfo();
					c_state = Connecting_Refreshing;
				}
			}

			if (c_state == Connecting_Refreshing) {
				if ((now - c_timestamp) > std::chrono::seconds(1)) {
					if (info.empty()) {
						c_state = Connecting_Nothing;
					}
					else {
						//Finding last ping server
						std::size_t min_ping_i = 0;
						std::uint32_t min_ping = info[min_ping_i].ping;
						for (std::size_t i = 1; i < info.size(); i++) {
							if (info[i].ping < min_ping && !info[i].passwordProtected) {
								min_ping = info[i].ping;
								min_ping_i = i;
							}
						}
						
						//And finally connecting
						const Biribit::ServerInfo& min = info[min_ping_i];
						client->Connect(min.addr.c_str(), min.port);
						c_timestamp = now;
						c_state = Connecting_Connecting;
					}
				}
			}

			if (c_state == Connecting_Connecting) {
				if ((now - c_timestamp) > std::chrono::seconds(3)) {
					c_state = Connecting_Nothing;
				}
			}

			if (c_state == Connecting_Disconnecting) {
				c_state = Connecting_Nothing;
			}
		}
	}
}