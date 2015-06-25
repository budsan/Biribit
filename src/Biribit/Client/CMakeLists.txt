cmake_minimum_required(VERSION 2.8.3)

set(INCROOT ${PROJECT_SOURCE_DIR}/include/Biribit/Client)
set(SRCROOT ${PROJECT_SOURCE_DIR}/src/Biribit/Client)

include_directories(
    ${BIRIBIT_RAKNET_INCLUDE_PATH}
)

set(SRC
	${SRCROOT}/ConnectionImpl.cpp
	${SRCROOT}/ConnectionImpl.h
	${SRCROOT}/ServerInfoImpl.cpp
	${SRCROOT}/ServerInfoImpl.h
	${SRCROOT}/BiribitClient.cpp
	${INCROOT}/BiribitClient.h
	${SRCROOT}/BiribitTypes.cpp
	${INCROOT}/BiribitTypes.h
	${SRCROOT}/BiribitClientImpl.cpp
	${SRCROOT}/BiribitClientImpl.h
	${SRCROOT}/BiribitClientExports.cpp
	${INCROOT}/BiribitClientExports.h
)

add_library(BiribitClient SHARED ${SRC})

target_link_libraries(BiribitClient
	BiribitCommon
	ProtoMessages
	RakNetLibStatic
)
