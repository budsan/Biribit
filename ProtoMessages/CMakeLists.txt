cmake_minimum_required(VERSION 2.8)

if(SYS_OS_WINDOWS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/protobuf/vsprojects)
    set(PROTOBUF_PLATFORM_SRC_FILES
        protobuf/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
        protobuf/src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
        protobuf/src/google/protobuf/stubs/stringprintf.cc
        protobuf/src/google/protobuf/arena.cc
        protobuf/src/google/protobuf/arenastring.cc
    )
elseif (SYS_OS_ANDROID)
    set(PROTOBUF_PROTOC_EXECUTABLE /usr/bin/protoc)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/android/)
    set(PROTOBUF_PLATFORM_SRC_FILES
        protobuf/src/google/protobuf/stubs/stringprintf.cc
        protobuf/src/google/protobuf/arena.cc
        protobuf/src/google/protobuf/arenastring.cc
    )
    add_definitions(-DGOOGLE_PROTOBUF_NO_RTTI -Wno-sign-compare -Wno-unused-parameter -Wno-sign-promo)
else()
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/protobuf/)
    set(PROTOBUF_PLATFORM_SRC_FILES
        protobuf/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
        protobuf/src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
        protobuf/src/google/protobuf/stubs/stringprintf.cc
        protobuf/src/google/protobuf/arena.cc
        protobuf/src/google/protobuf/arenastring.cc
    )
endif()
set(PROTOBUF_COMMON_SRC_FILES
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
)

set(PROTOBUF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/src)

include(FindProtobuf)
find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})
    
FILE(GLOB ALL_PROTO_SRCS *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${ALL_PROTO_SRCS})

add_library(ProtoMessages STATIC
    ${PROTOBUF_COMMON_SRC_FILES}
    ${PROTOBUF_PLATFORM_SRC_FILES}
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)


