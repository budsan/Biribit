cmake_minimum_required(VERSION 2.8)

if(SYS_OS_WINDOWS)
	if(ARCH_32BITS)
		set(PROTOBUF_LIBRARY ${PROJECT_BINARY_DIR}/lib/Win32)
	elseif(ARCH_64BITS)
		set(PROTOBUF_LIBRARY ${PROJECT_BINARY_DIR}/lib/x64)
	endif()
	set(PROTOBUF_LIBRARIES libprotobuf)
endif()

set(PROTOBUF_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/ProtoMessages/protobuf/src)

include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})
	
FILE(GLOB ALL_PROTO_SRCS *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${ALL_PROTO_SRCS})

add_definitions(-DGOOGLE_PROTOBUF_NO_RTTI)
add_library(ProtoMessages STATIC
	protobuf/src/google/protobuf/stubs/common.cc
	protobuf/src/google/protobuf/stubs/once.cc
	protobuf/src/google/protobuf/extension_set.cc
	protobuf/src/google/protobuf/generated_message_util.cc
	protobuf/src/google/protobuf/message_lite.cc
	protobuf/src/google/protobuf/repeated_field.cc
	protobuf/src/google/protobuf/wire_format_lite.cc
	protobuf/src/google/protobuf/io/coded_stream.cc
	protobuf/src/google/protobuf/io/zero_copy_stream.cc
	protobuf/src/google/protobuf/io/zero_copy_stream_impl_lite.cc
	protobuf/src/google/protobuf/stubs/strutil.cc
	protobuf/src/google/protobuf/stubs/substitute.cc
	protobuf/src/google/protobuf/stubs/structurally_valid.cc
	protobuf/src/google/protobuf/descriptor.cc
	protobuf/src/google/protobuf/descriptor.pb.cc
	protobuf/src/google/protobuf/descriptor_database.cc
	protobuf/src/google/protobuf/dynamic_message.cc
	protobuf/src/google/protobuf/extension_set_heavy.cc
	protobuf/src/google/protobuf/generated_message_reflection.cc
	protobuf/src/google/protobuf/message.cc
	protobuf/src/google/protobuf/reflection_ops.cc
	protobuf/src/google/protobuf/service.cc
	protobuf/src/google/protobuf/text_format.cc
	protobuf/src/google/protobuf/unknown_field_set.cc
	protobuf/src/google/protobuf/wire_format.cc
	protobuf/src/google/protobuf/io/gzip_stream.cc
	protobuf/src/google/protobuf/io/printer.cc
	protobuf/src/google/protobuf/io/tokenizer.cc 
	protobuf/src/google/protobuf/io/zero_copy_stream_impl.cc
	protobuf/src/google/protobuf/compiler/importer.cc
	protobuf/src/google/protobuf/compiler/parser.cc
	${PROTO_SRCS}
	${PROTO_HDRS}
)

