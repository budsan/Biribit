cmake_minimum_required(VERSION 2.8)

add_executable(BiribitServer
	${SYSTEM_LIB_CPPS}
	RakNetServer.cpp
	main.cpp
)

if(SYS_OS_WINDOWS)
	set(SERVER_LIBRARIES)
elseif(SYS_OS_LINUX)
	set(SERVER_LIBRARIES rt pthread)
endif()

target_link_libraries(BiribitServer
	${SERVER_LIBRARIES}
	BiribitCommon
	ProtoMessages
	RakNetLibStatic
)
