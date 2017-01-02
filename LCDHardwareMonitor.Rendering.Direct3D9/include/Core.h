///
// Fundamental Types
///

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  r32;
typedef double r64;

typedef char    c8;
typedef wchar_t c16;


///
// Helpful Macros & Templates
///

#define Kilobyte 1024LL
#define Megabyte 1024LL * Kilobyte
#define Gigabyte 1024LL * Megabyte

#define Assert(condition) if (!(condition)) { *((u8 *) 0) = 0; }
#define InvalidCodePath Assert(!"Invalid code path")
#define InvalidDefaultCase default: InvalidCodePath; break

template<typename T, size_t S>
inline size_t ArrayCount(const T (&arr)[S])
{
	return S;
}

template<typename T, size_t S>
inline size_t ArraySize(const T (&arr)[S])
{
	return S * sizeof(T);
}


///
// Global Functionality
///
//TODO: Disable exceptions?
//#define _HAS_EXCEPTIONS 0

using namespace std;
#include "Logging.hpp"

#include <memory>
using std::unique_ptr;


///
// Math
///

i32
Clamp(i32 value, i32 min, i32 max)
{
	if (value < min) value = min;
	if (value > max) value = max;

	return value;
}

i64
Clamp(i64 value, i64 min, i64 max)
{
	if (value < min) value = min;
	if (value > max) value = max;

	return value;
}

struct V2i
{
	i32 x;
	i32 y;
};

//Operators
inline bool
operator== (V2i lhs, V2i rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline bool
operator!= (V2i lhs, V2i rhs)
{
	return !(lhs.x == rhs.x && lhs.y == rhs.y);
}

inline V2i
operator+ (V2i lhs, V2i rhs)
{
	return {lhs.x + rhs.x, lhs.y + rhs.y};
}

inline V2i
operator- (V2i lhs, V2i rhs)
{
	return {lhs.x - rhs.x, lhs.y - rhs.y};
}

inline V2i
operator* (V2i v, i32 multiplier)
{
	return {multiplier * v.x, multiplier * v.y};
}

inline V2i
operator/ (V2i v, i32 dividend)
{
	return {v.x / dividend, v.y / dividend};
}

inline void
Clamp(V2i v, V2i maxSize)
{
	if (v.x > maxSize.x) v.x = maxSize.x;
	if (v.y > maxSize.y) v.y = maxSize.y;
}


union V3
{
	struct
	{
		r32 x;
		r32 y;
		r32 z;
	};

	//Aliases
	struct
	{
		r32 r;
		r32 g;
		r32 b;
	};
	struct
	{
		r32 radius;
		r32 theta;
		r32 phi;
	};
	r32 arr[3];
};

//Operators
inline bool
operator== (V3 lhs, V3 rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

inline bool
operator!= (V3 lhs, V3 rhs)
{
	return !(lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z);
}

inline V3
operator+ (V3 lhs, V3 rhs)
{
	return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

inline V3
operator- (V3 lhs, V3 rhs)
{
	return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

inline V3
operator* (V3 v, r32 multiplier)
{
	return {multiplier * v.x, multiplier * v.y, multiplier * v.z};
}

inline V3
operator/ (V3 v, r32 dividend)
{
	return {v.x / dividend, v.y / dividend, v.z / dividend};
}

inline void
Clamp(V3 v, V3 maxSize)
{
	if (v.x > maxSize.x) v.x = maxSize.x;
	if (v.y > maxSize.y) v.y = maxSize.y;
	if (v.z > maxSize.z) v.y = maxSize.z;
}


///
// Assets
///
enum Mesh
{
	Null,
	Quad,
	Sphere,
	Cube,
	Cylinder,
	Grid,

	Count
};