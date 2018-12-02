#ifndef LHM_COMMON
#define LHM_COMMON

#include <cstdint>
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using r32 = float;
using r64 = double;

using c8  = char;
using c16 = wchar_t;

using b8  = bool;
using b32 = int;

using size = size_t;

const u64 Kilobyte = 1024LL;
const u64 Megabyte = 1024LL * Kilobyte;
const u64 Gigabyte = 1024LL * Megabyte;

#if DEBUG
	#define Assert(condition) if (!(condition)) { *((u8 *) 0) = 0; }
#else
	#define Assert(condition)
#endif

#define nameof(x) #x
#define IGNORE
#define UNUSED(x) (x);
#define HAS_FLAG(x, f) ((x & f) == f)
#define IF(expression, ...) if (expression) { __VA_ARGS__; }

template<typename T, size_t S>
inline size_t ArrayLength(const T(&arr)[S]) { UNUSED(arr); return S; }

template<typename T, size_t S>
inline size_t ArraySize(const T(&arr)[S]) { UNUSED(arr); return S * sizeof(T); }

#endif
