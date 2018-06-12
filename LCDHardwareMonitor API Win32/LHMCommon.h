#ifndef LHMCOMMON
#define LHMCOMMON

#include <cstdint>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  r32;
typedef double r64;

typedef char    c8;
typedef wchar_t c16;

typedef bool b8;
typedef int  b32;

//TODO: Settle on a convention
typedef size_t size;
//typedef size_t index;
//typedef size_t ptrsize;

#define Kilobyte 1024LL
#define Megabyte 1024LL * Kilobyte
#define Gigabyte 1024LL * Megabyte

//#if EXPORTING
//	#define LHM_API extern "C" __declspec(dllexport)
//#else
//	#define LHM_API extern "C" __declspec(dllimport)
//#endif

#if DEBUG
	#define Assert(condition) if (!(condition)) { *((u8 *) 0) = 0; }
#else
	#define Assert(condition)
#endif

#if __cplusplus_cli
	#define PUBLIC public
#else
	#define PUBLIC
#endif

#define nameof(x) #x

#define IF(expression, ...) if (expression) { __VA_ARGS__; }

#endif
