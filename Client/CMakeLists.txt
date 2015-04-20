cmake_minimum_required(VERSION 2.8)

link_directories(
        ${PROJECT_BINARY_DIR}/lib
        /usr/lib)

add_library(BiribitClient SHARED
        ${SYSTEM_LIB_CPPS}
        ../Common/Packet.cpp
        ../Common/PrintLog.cpp
        BiribitClient.cpp
        BiribitClientExports.cpp
)

target_link_libraries(BiribitClient
	RakNetLibStatic
        rt
        pthread
	ncurses
)
