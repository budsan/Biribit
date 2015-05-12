#pragma once
#include <cstdio>
#include <cassert>
#include <cstdlib>

#ifdef WIN32
#define __current__func__ __FUNCTION__
#else
#define __current__func__ __func__
#endif

#ifdef _DEBUG
#define BIRIBIT_ASSERT(expression) assert(expression)
#else
#define BIRIBIT_ASSERT(expression)
#endif

// Error macro.
#ifdef BIRIBIT_ERRORS_AS_WARNINGS
#define BIRIBIT_ERROR GUY_WARN
#else
#define BIRIBIT_ERROR(...) do \
	{ \
	printLog("%s -- ", __current__func__); \
	printLog(__VA_ARGS__); \
	printLog("\n"); \
	assert(0); \
	std::exit(-1); \
	} while (0)
#endif

// Warning macro.
#define BIRIBIT_WARN(...) do \
	{ \
	printLog("%s -- ", __current__func__); \
	printLog(__VA_ARGS__); \
	printLog("\n"); \
	} while (0)

