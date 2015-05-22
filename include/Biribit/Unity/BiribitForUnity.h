#pragma once

#include <Biribit/Client.h>
#include <memory>

API_EXPORT std::shared_ptr<Biribit::Client>& BiribitForUnity_GetClient();

API_C_EXPORT void BiribitForUnity_Connect(const char* _clientname, const char* _appid);
API_C_EXPORT void BiribitForUnity_Disconnect();
API_C_EXPORT int BiribitForUnity_IsConnected();

API_C_EXPORT void BiribitForUnity_JoinOrCreateRoom(int num_slots, int slot_to_join_id);
API_C_EXPORT int BiribitForUnity_JoinedRoomId();
API_C_EXPORT int BiribitForUnity_JoinedRoomSlot();

API_C_EXPORT int BiribitForUnity_GetDataSizeOfNextReceived();
API_C_EXPORT int BiribitForUnity_PullReceived(void* data, int length, int* slot_id);

API_C_EXPORT void BiribitForUnity_Update();