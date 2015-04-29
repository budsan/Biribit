cmake_minimum_required(VERSION 2.8)

add_library(BiribitClient SHARED
	${SYSTEM_LIB_CPPS}
	../Common/Packet.cpp
	../Common/PrintLog.cpp
	../Common/Generic.cpp
	BiribitClient.cpp
	BiribitClientExports.cpp
)

if(SYS_OS_WINDOWS)
	set(CLIENT_LIBRARIES)
elseif(SYS_OS_LINUX)
	set(CLIENT_LIBRARIES rt pthread GL)
endif()

target_link_libraries(BiribitClient
	${CLIENT_LIBRARIES}
	ProtoMessages
	RakNetLibStatic
)
