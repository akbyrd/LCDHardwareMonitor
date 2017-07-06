#pragma once

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

//@OTODO: Can _cdecl be added here? Seems like it needs to be after the return type
#if EXPORTING
	#define LHM_API extern "C" __declspec(dllexport)
#else
	#define LHM_API extern "C" __declspec(dllimport)
#endif

#if DEBUG
	#define Assert(condition) if (!(condition)) { *((u8 *) 0) = 0; }
#else
	#define Assert(condition)
#endif

#define nameof(x) #x

template<typename T, i32 S>
inline i32 ArrayCount(const T(&arr)[S])
{
	return S;
}

template<typename T, i32 S>
inline i32 ArraySize(const T(&arr)[S])
{
	return S * sizeof(T);
}

//struct String
//{
//	i32 length;
//	c16* items;
//};

template<typename T>
struct List
{
	//TODO: bracket operator
	i32 length;
	i32 capacity;
	T*  items;
};

template<typename T>
inline List<T>
List_Create(i32 capacity = 0)
{
	List<T> list;

	list.length   = 0;
	list.capacity = capacity;
	list.items    = (T*) malloc(sizeof(T) * capacity);

	return list;
}

template<typename T>
inline void
List_Append(List<T>& list, T& item)
{
	if (list.length == list.capacity)
	{
		list.capacity = list.capacity ? 2*list.capacity : 4;
		list.items = (T*) realloc(list.items, sizeof(T) * list.capacity);
	}
	list.items[list.length++] = item;
}

template<typename T>
inline void
List_Clear(List<T>& list)
{
	list.length = 0;
}

template<typename T>
inline void
List_Free(List<T>& list)
{
	free(list.items);
	list = {};
}

//Named varargs sure would be nice...
#define IF(expression, ...) \
if (expression)             \
{                           \
	__VA_ARGS__;            \
}

/* NOTE: Raise a compiler error when switching over
 * an enum and any enum values are missing a case.
 * https://msdn.microsoft.com/en-us/library/fdt9w8tf.aspx
 */
#pragma warning (error: 4062)

#include <memory>
using std::unique_ptr;