#pragma once

#if defined OS_WINDOWS
#define HELPER_DLL_EXPORT __declspec(dllexport)
#define HELPER_DLL_IMPORT __declspec(dllimport)
#elif defined OS_LINUX
#define HELPER_DLL_EXPORT __attribute__ ((visibility ("default")))
#define HELPER_DLL_IMPORT __attribute__ ((visibility ("default")))
#else
#define HELPER_DLL_EXPORT
#define HELPER_DLL_IMPORT
#endif

#ifdef RAVEN_SERIALIZE_EXPORTS
#define RAVEN_SER_API HELPER_DLL_EXPORT
#else
#define RAVEN_SER_API HELPER_DLL_IMPORT
#endif
