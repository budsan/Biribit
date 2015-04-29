cmake_minimum_required(VERSION 2.8)

if(SYS_OS_WINDOWS)
	if(ARCH_32BITS)
		set(PROTOBUF_LIBRARY ${PROJECT_SOURCE_DIR}/lib/Win32)
	elseif(ARCH_64BITS)
		set(PROTOBUF_LIBRARY ${PROJECT_SOURCE_DIR}/lib/x64)
	endif()
endif()

set(PROTOBUF_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/ProtoMessages/protobuf/src)

include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

FILE(GLOB ALL_PROTO_SRCS *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${ALL_PROTO_SRCS})
add_library(ProtoMessages
	${PROTO_SRCS}
	${PROTO_HDRS}
)

target_link_libraries(ProtoMessages ${PROTOBUF_LIBRARIES})

