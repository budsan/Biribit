cmake_minimum_required(VERSION 2.8)

FILE(GLOB ALL_HEADER_SRCS *.h)
FILE(GLOB ALL_CPP_SRCS *.cpp)

add_library(RakNetLibStatic STATIC ${ALL_CPP_SRCS} ${ALL_HEADER_SRCS})


