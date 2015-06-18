cmake_minimum_required(VERSION 2.8.3)

add_library(BiribitCommon STATIC
	BiribitMessageIdentifiers.h
	Debug.h
	Generic.cpp
	Generic.h
	Packet.cpp
	PrintLog.cpp
	PrintLog.h
	RefSwap.h
	TaskPool.h
	Types.h
)

