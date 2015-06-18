cmake_minimum_required(VERSION 2.8.3)

set(INCROOT ${PROJECT_SOURCE_DIR}/include/Biribit/Unity)
set(SRCROOT ${PROJECT_SOURCE_DIR}/src/Biribit/Unity)

set(SRC
	${INCROOT}/BiribitForUnity.h
	${SRCROOT}/BiribitForUnity.cpp
)

add_library(BiribitForUnity SHARED ${SRC})

target_link_libraries(BiribitForUnity
	BiribitClient
	BiribitCommon
)
