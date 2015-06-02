#ifndef _CONFIG_H_
#define _CONFIG_H_

////////////////////////////////////////////////////////////
// Identify the operating system
// see http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
////////////////////////////////////////////////////////////
#if defined(_WIN32)

	// Windows
	#define SYSTEM_WINDOWS
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#pragma comment(linker,"/NODEFAULTLIB:atlthunk.lib")

#elif defined(__APPLE__) && defined(__MACH__)

	// Apple platform, see which one it is
	#include "TargetConditionals.h"

	#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR

		// iOS
		#define SYSTEM_IOS

	#elif TARGET_OS_MAC

		// MacOS
		#define SYSTEM_MACOS

	#else

		// Unsupported Apple system
		#error This Apple operating system is not supported by System library

	#endif

#elif defined(__unix__)

	// UNIX system, see which one it is
	#if defined(__ANDROID__)

		// Android
		#define SYSTEM_ANDROID

	#elif defined(__linux__)

			// Linux
		#define SYSTEM_LINUX

	#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)

		// FreeBSD
		#define SYSTEM_FREEBSD

	#else

		// Unsupported UNIX system
		#error This UNIX operating system is not supported by System library

	#endif

#else

	// Unsupported system
	#error This operating system is not supported by System library

#endif


////////////////////////////////////////////////////////////
// Define a portable debug macro
////////////////////////////////////////////////////////////
#if !defined(NDEBUG)

	#define DEBUG

#endif

#undef BIRIBIT_DISALLOW_EVIL_CONSTRUCTORS
#define BIRIBIT_DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
  TypeName(const TypeName&) = delete;                   \
  TypeName& operator=(const TypeName&) = delete

////////////////////////////////////////////////////////////
// Define helpers to create portable import / export macros for each module
////////////////////////////////////////////////////////////
#if !defined(STATIC)

	#if defined(SYSTEM_WINDOWS)

		// Windows compilers need specific (and different) keywords for export and import
		#define API_EXPORT __declspec(dllexport)
		#define API_IMPORT __declspec(dllimport)

		#define STDCALL __stdcall

		// For Visual C++ compilers, we also need to turn off this annoying C4251 warning
		#ifdef _MSC_VER

			#pragma warning(disable : 4251)

		#endif

	#else // Linux, FreeBSD, Mac OS X

		#if __GNUC__ >= 4

			// GCC 4 has special keywords for showing/hidding symbols,
			// the same keyword is used for both importing and exporting
			#define API_EXPORT __attribute__ ((__visibility__ ("default")))
			#define API_IMPORT __attribute__ ((__visibility__ ("default")))

		#else

			// GCC < 4 has no mechanism to explicitely hide symbols, everything's exported
			#define API_EXPORT
			#define API_IMPORT

		#endif

	#define STDCALL __attribute__((stdcall))

	#endif

#else

	// Static build doesn't need import/export macros
	#define API_EXPORT
	#define API_IMPORT

	#define STDCALL

#endif

#define API_C_EXPORT extern "C" API_EXPORT

#endif // CONFIG_H_
