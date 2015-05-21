#include <Biribit/Client.h>
#include <Biribit/Common/PrintLog.h>
#include <memory>
#include <cstdarg>

namespace priv
{
	std::unique_ptr<Biribit::Client> client;
}

std::unique_ptr<Biribit::Client>& GetClient()
{
	if (priv::client == nullptr)
		priv::client = std::unique_ptr<Biribit::Client>(new Biribit::Client());

	return priv::client;
}


API_C_EXPORT void BiribitForUnity_Connect(const char* clientname, const char* appid)
{

}

API_C_EXPORT void BiribitForUnity_Disconnect()
{

}

API_C_EXPORT int BiribitForUnity_IsConnected()
{
	return 0;
}

API_C_EXPORT void BiribitForUnity_JoinOrCreateRoom(int num_slots, int slot_to_join_id)
{

}

API_C_EXPORT int BiribitForUnity_JoinedRoomId()
{
	return 0;
}

API_C_EXPORT void BiribitForUnity_SendToRoom(const char* data, int length, int reliability)
{

}