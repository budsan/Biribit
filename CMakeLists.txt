cmake_minimum_required(VERSION 2.8.3)

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

# set Protocol buffer specific options

sys_set_option(PROTOC_EXECUTABLE "/usr/bin/protoc" PATH "Path to the Protoc executable")

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

set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

if(NOT (SYS_OS_IOS OR SYS_OS_ANDROID))
    sys_set_option(BIRIBIT_BUILD_SERVER TRUE BOOL "TRUE to build the Biribit Server, FALSE to do not")
    sys_set_option(BIRIBIT_BUILD_TEST TRUE BOOL "TRUE to build the Biribit Test Client, FALSE to do not")
else()
    set(BIRIBIT_BUILD_SERVER FALSE)
    set(BIRIBIT_BUILD_TEST FALSE)
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
    set(CMAKE_INSTALL_PREFIX ${ANDROID_NDK}/sources/${PROJECT_NAME})

    # we install libs in a subdirectory named after the ABI (lib/mips/*.so)
    set(LIB_SUFFIX "/${ANDROID_ABI}")

    # pass shared STL configuration (if any)
    if (ANDROID_STL MATCHES "_shared")
        add_definitions("-DSTL_LIBRARY=${ANDROID_STL}")
    endif()

    # this is a workaround to compile activity without the stl library as a dependency
    # we save the original compilation command line to restore it later in Macro.cmake
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY_WITH_STL ${CMAKE_CXX_CREATE_SHARED_LIBRARY})
    set(CMAKE_CXX_CREATE_SHARED_LIBRARY_WITHOUT_STL "<CMAKE_CXX_COMPILER> <CMAKE_SHARED_LIBRARY_CXX_FLAGS> <LANGUAGE_COMPILE_FLAGS> <LINK_FLAGS> <CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS> <SONAME_FLAG><TARGET_SONAME> -o <TARGET> <OBJECTS> <LINK_LIBRARIES>")
else()
    unset(ANDROID_ABI CACHE)
    unset(ANDROID_API_MIN CACHE)
    unset(ANDROID_STL CACHE)
    unset(ANDROID_NATIVE_API_LEVEL CACHE)
    unset(ANDROID_NDK CACHE)
endif()

# enable project folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

if(SYS_OS_WINDOWS)
	include_directories(${CMAKE_CURRENT_SOURCE_DIR}/TestClient/include)
	if(ARCH_32BITS)
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/Win32)
	elseif(ARCH_64BITS)
		link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/x64)
	endif()
elseif(SYS_OS_LINUX)
	link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/Linux /usr/lib)
endif()

# remove SL security warnings with Visual C++
if(SYS_COMPILER_MSVC)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
endif()

if(SYS_COMPILER_GCC)
    add_definitions(-std=c++11)
#-----------------------------------------------------------------------------
#
# To fix compilation problem: relocation R_X86_64_32 against `a local symbol' can not be
# used when making a shared object; recompile with -fPIC
# See http://www.cmake.org/pipermail/cmake/2007-May/014350.html
#
    IF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
        ADD_DEFINITIONS(-fPIC)
    ENDIF( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )	
endif()

set(PROJECT_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin/)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_OUTPUT_PATH} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${PROJECT_OUTPUT_PATH} CACHE PATH "Build directory" FORCE)

SET(BIRIBIT_RAKNET_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/RakNet/RakNet/Source)
add_subdirectory(RakNet)
add_subdirectory(src/Biribit)

if(BIRIBIT_BUILD_TEST) 
	add_subdirectory(TestClient)
endif()
