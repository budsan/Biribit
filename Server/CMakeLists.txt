cmake_minimum_required(VERSION 2.8)

link_directories(
        ${PROJECT_BINARY_DIR}/lib
        /usr/lib)

add_executable(BiribitServer.exe
        ${SYSTEM_LIB_CPPS}
        ../Common/Packet.cpp
	../Common/PrintLog.cpp
	../Common/Generic.cpp
	RakNetServer.cpp
	main.cpp
)

target_link_libraries(BiribitServer.exe
	ProtoMessages
	RakNetLibStatic
        rt
        pthread
)
