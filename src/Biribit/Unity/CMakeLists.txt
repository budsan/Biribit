cmake_minimum_required(VERSION 2.8.3)

add_library(BiribitForUnity SHARED
	BiribitForUnity.cpp
)

target_link_libraries(BiribitForUnity
	BiribitClient
	BiribitCommon
)
