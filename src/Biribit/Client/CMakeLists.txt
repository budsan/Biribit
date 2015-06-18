cmake_minimum_required(VERSION 2.8.3)

include_directories(
    ${BIRIBIT_RAKNET_INCLUDE_PATH}
)

add_library(BiribitClient SHARED
	ServerConnection.cpp
	ServerInfo.cpp
	BiribitClient.cpp
	BiribitClientTypes.cpp
	BiribitClientImpl.cpp
	BiribitClientExports.cpp
)

target_link_libraries(BiribitClient
	BiribitCommon
	ProtoMessages
	RakNetLibStatic
)
