cmake_minimum_required(VERSION 2.8.3)

include(FindProtobuf)
find_package(Protobuf REQUIRED)

if("${PROTOBUF_PROTOC_EXECUTABLE}" STREQUAL "PROTOBUF_PROTOC_EXECUTABLE-NOTFOUND")
	set(PROTOBUF_PROTOC_EXECUTABLE "${PROTOC_EXECUTABLE}")
endif()

if(SYS_OS_WINDOWS)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/protobuf/vsprojects)
    set(PROTOBUF_PLATFORM_SRC_FILES)
elseif (SYS_OS_ANDROID)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/android/)
    set(PROTOBUF_PLATFORM_SRC_FILES)
    add_definitions(-DGOOGLE_PROTOBUF_NO_RTTI -Wno-sign-compare -Wno-unused-parameter -Wno-sign-promo)
else()
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/protobuf/)
    set(PROTOBUF_PLATFORM_SRC_FILES)
endif()
set(PROTOBUF_COMMON_SRC_FILES
  protobuf/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc 
  protobuf/src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
  protobuf/src/google/protobuf/stubs/bytestream.cc
  protobuf/src/google/protobuf/stubs/bytestream.h 
  protobuf/src/google/protobuf/stubs/common.cc
  protobuf/src/google/protobuf/stubs/hash.h   
  protobuf/src/google/protobuf/stubs/map_util.h   
  protobuf/src/google/protobuf/stubs/mathutil.h   
  protobuf/src/google/protobuf/stubs/once.cc  
  protobuf/src/google/protobuf/stubs/shared_ptr.h 
  protobuf/src/google/protobuf/stubs/status.cc
  protobuf/src/google/protobuf/stubs/status.h 
  protobuf/src/google/protobuf/stubs/status_macros.h
  protobuf/src/google/protobuf/stubs/statusor.cc  
  protobuf/src/google/protobuf/stubs/statusor.h   
  protobuf/src/google/protobuf/stubs/stringpiece.cc 
  protobuf/src/google/protobuf/stubs/stringpiece.h
  protobuf/src/google/protobuf/stubs/stringprintf.cc
  protobuf/src/google/protobuf/stubs/stringprintf.h 
  protobuf/src/google/protobuf/stubs/strutil.cc   
  protobuf/src/google/protobuf/stubs/strutil.h
  protobuf/src/google/protobuf/stubs/time.cc  
  protobuf/src/google/protobuf/stubs/time.h   
  protobuf/src/google/protobuf/arena.cc   
  protobuf/src/google/protobuf/arenastring.cc 
  protobuf/src/google/protobuf/extension_set.cc   
  protobuf/src/google/protobuf/generated_message_util.cc
  protobuf/src/google/protobuf/message_lite.cc
  protobuf/src/google/protobuf/repeated_field.cc  
  protobuf/src/google/protobuf/wire_format_lite.cc
  protobuf/src/google/protobuf/io/coded_stream.cc 
  protobuf/src/google/protobuf/io/coded_stream_inl.h
  protobuf/src/google/protobuf/io/zero_copy_stream.cc   
  protobuf/src/google/protobuf/io/zero_copy_stream_impl_lite.cc
)

set(PROTOBUF_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/protobuf/src)
include_directories(${PROTOBUF_INCLUDE_DIR})

FILE(GLOB ALL_PROTO_SRCS *.proto)
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${ALL_PROTO_SRCS})

add_library(ProtoMessages STATIC
    ${PROTOBUF_COMMON_SRC_FILES}
    ${PROTOBUF_PLATFORM_SRC_FILES}
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)


