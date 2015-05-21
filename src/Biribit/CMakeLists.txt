cmake_minimum_required(VERSION 2.8.3)

include_directories(
	${PROJECT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/ProtoMessages/protobuf/src
	${CMAKE_CURRENT_BINARY_DIR}/ProtoMessages
)

add_subdirectory(ProtoMessages)
add_subdirectory(Common)
add_subdirectory(Client)
add_subdirectory(Unity)

if (BIRIBIT_BUILD_SERVER)
    add_subdirectory(Server)
endif()