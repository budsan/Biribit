cmake_minimum_required(VERSION 2.8)

add_executable(BiribitServer.exe
	${SYSTEM_LIB_CPPS}
	../Common/Packet.cpp
	../Common/PrintLog.cpp
	../Common/Generic.cpp
	RakNetServer.cpp
	main.cpp
)

if(SYS_OS_WINDOWS)
	set(SERVER_LIBRARIES)
elseif(SYS_OS_LINUX)
	set(SERVER_LIBRARIES rt pthread GL)
endif()

target_link_libraries(BiribitServer.exe
	${SERVER_LIBRARIES}
	ProtoMessages
	RakNetLibStatic
)
