cmake_minimum_required(VERSION 2.8)

project(Biribit)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-std=c++11)
endif()

# with -fPIC
if(UNIX AND NOT WIN32 AND NOT ANDROID)
  FIND_PROGRAM(CMAKE_UNAME uname /bin /usr/bin /usr/local/bin )
  if(CMAKE_UNAME)
    EXEC_PROGRAM(uname ARGS -m OUTPUT_VARIABLE CMAKE_SYSTEM_PROCESSOR)
    SET(CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR} CACHE INTERNAL
"processor type (i386 and x86_64)")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
      ADD_DEFINITIONS(-fPIC)
    endif()
  endif()
endif()

include_directories(
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/Common
	${PROJECT_SOURCE_DIR}/RakNet
)

add_subdirectory(RakNet)
add_subdirectory(Server)
add_subdirectory(Client)
add_subdirectory(TestClient)
