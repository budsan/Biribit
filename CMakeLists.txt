cmake_minimum_required(VERSION 2.8)

# define a macro that helps defining an option
macro(sys_set_option var default type docstring)
    if(NOT DEFINED ${var})
        set(${var} ${default})
    endif()
    set(${var} ${${var}} CACHE ${type} ${docstring} FORCE)
endmacro()

# these options have to be set before CMake detects/configures the toolchain

# determine whether to create a debug or release build
sys_set_option(CMAKE_BUILD_TYPE Release STRING "Choose the type of build (Debug or Release)")

# set Android specific options

# define the minimum API level to be used
sys_set_option(ANDROID_API_MIN 9 STRING "Choose the Android API level to be used (minimum 9)")
# mirror the setting for the toolchain file
set(ANDROID_NATIVE_API_LEVEL ${ANDROID_API_MIN})

# define the path to the Android NDK
sys_set_option(ANDROID_NDK "$ENV{ANDROID_NDK}" PATH "Path to the Android NDK")

# define the STL implementation to be used
sys_set_option(ANDROID_STL c++_shared STRING "Choose the STL implementation to be used (experimental)")

# default the ABI to ARM v7a for hardware floating point
if(NOT ANDROID_ABI)
    set(ANDROID_ABI armeabi-v7a)
endif()

project(Biribit)

# include the configuration file
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-std=c++11)
endif()

#-----------------------------------------------------------------------------
#
# To fix compilation problem: relocation R_X86_64_32 against `a local symbol' can not be
# used when making a shared object; recompile with -fPIC
# See http://www.cmake.org/pipermail/cmake/2007-May/014350.html
#
IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
	ADD_DEFINITIONS(-fPIC)
ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )

include_directories(
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/Common
	${PROJECT_SOURCE_DIR}/ProtoMessages
	${PROJECT_SOURCE_DIR}/RakNet/RakNet/Source
	${PROJECT_SOURCE_DIR}/Client
)

if(SYS_OS_WINDOWS)
	include_directories(
		${PROJECT_SOURCE_DIR}/TestClient/include
		${PROJECT_SOURCE_DIR}/ProtoMessages/protobuf/src
	)
endif()

if(SYS_OS_WINDOWS)
	if(ARCH_32BITS)
		link_directories(${PROJECT_BINARY_DIR}/lib/Win32)
	elseif(ARCH_64BITS)
		link_directories(${PROJECT_BINARY_DIR}/lib/x64)
	endif()
elseif(SYS_OS_LINUX)
	link_directories(${PROJECT_BINARY_DIR}/lib/Linux /usr/lib)
endif()

# Android options
if(SYS_OS_ANDROID)
    # make sure there's the android library available
    if (${ANDROID_API_MIN} LESS 9)
        message(FATAL_ERROR "Android API level must be equal or greater than 9. Please adjust the CMake variable 'ANDROID_API_MIN'.")
    endif()

    if(NOT ANDROID_NDK)
        message(FATAL_ERROR "The Android NDK couldn't be found. Please adjust the CMake variable 'ANDROID_NDK' to point to the NDK directory.")
    endif()

    # CMake doesn't support defining the STL to be used with Nsight Tegra, so warn the user
    if(CMAKE_VS_PLATFORM_NAME STREQUAL "Tegra-Android")
        message(WARNING "CMake might not properly support setting the STL. Make sure to adjust all generated library projects!")
    endif()

    # install everything in $NDK/sources/ because this path is appended by the NDK (convenient)
    set(CMAKE_INSTALL_PREFIX ${ANDROID_NDK}/sources/biribit)

    # we install libs in a subdirectory named after the ABI (lib/mips/*.so)
    set(LIB_SUFFIX "/${ANDROID_ABI}")

    # pass shared STL configuration (if any)
    if (ANDROID_STL MATCHES "_shared")
        add_definitions("-DSTL_LIBRARY=${ANDROID_STL}")
    endif()
else()
    unset(ANDROID_ABI CACHE)
    unset(ANDROID_API_MIN CACHE)
    unset(ANDROID_STL CACHE)
    unset(ANDROID_NATIVE_API_LEVEL CACHE)
    unset(ANDROID_NDK CACHE)
endif()

# remove SL security warnings with Visual C++
if(SYS_COMPILER_MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif()

add_subdirectory(RakNet)
add_subdirectory(ProtoMessages)
add_subdirectory(Server)
add_subdirectory(Client)
add_subdirectory(TestClient)
