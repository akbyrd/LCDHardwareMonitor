#ifndef LHM_COMMON
#define LHM_COMMON

#include <stdint.h>
#include <string.h>
#include <math.h>

// Fuck you, Microsoft
#pragma warning(push, 0)
// signed unsigned mismatch
#pragma warning(disable: 4365)
// TODO: Why can't this be <memory.h>?
#include <memory>
#pragma warning(pop)

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

using size = size_t; // TODO: Consider removing this

const u64 Kilobyte = 1024LL;
const u64 Megabyte = 1024LL * Kilobyte;
const u64 Gigabyte = 1024LL * Megabyte;

template<typename T, size S>
constexpr inline size ArrayLength(const T(&)[S]) { return S; }

template<typename T, size S>
constexpr inline size ArraySize(const T(&)[S]) { return S * sizeof(T); }

template<typename ...Args>
constexpr inline void Unused(Args const& ...) {}

#if DEBUG
	#define Assert(condition) if (!(condition)) { *((u8 *) 0) = 0; }
	#define AssertOrUnused(condition) Assert(condition)
#else
	#define Assert(condition)
	#define AssertOrUnused(condition) Unused(condition)
#endif

#define nameof(x) #x
#define IGNORE
#define HAS_FLAG(x, f) ((x & f) == f)
#define IF(expression, ...) if (expression) { __VA_ARGS__; }
#define LHMVersion 0x00000001

#endif
