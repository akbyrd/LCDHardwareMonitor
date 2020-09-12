#ifndef LHM_MATH
#define LHM_MATH

// Conventions:
//   Right handed world
//   +X right, +Y up, -Z forward
//   Vectors are row vectors
//   Matrices are applied from the right side of vectors
//   v' = v * M
//   Matrices store axes and translation in columns
//   Matrices are stored in column-major order
//   Matrices are indexed in column-major order
//   m12 = M[2][1] = y.z
//   Transformations are applied S then R then T

// TODO: Try to collapse the various r32/r64 duplication
// TODO: Make Sqrt constexpr
// TODO: Use vectors for color functions?
// TODO: Can't use operators from the VS Watch window if they are
// pass-by-value, but passing by reference requires const everywhere. Ugh.

// -------------------------------------------------------------------------------------------------
// General

constexpr i8  i8Min  = -0x80;                  // -128;
constexpr i8  i8Max  =  0x7F;                  //  127;
constexpr i16 i16Min = -0x8000;                // -32'768;
constexpr i16 i16Max =  0x7FFF;                //  32'767;
constexpr i32 i32Min =  0x8000'0000;           // -2'147'483'648;
constexpr i32 i32Max =  0x7FFF'FFFF;           //  2'147'483'647;
constexpr i64 i64Min =  0x8000'0000'0000'0000; // -9'223'372'036'854'775'808;
constexpr i64 i64Max =  0x7FFF'FFFF'FFFF'FFFF; //  9'223'372'036'854'775'807;

constexpr u8  u8Max  =  0xFF;                  //  255;
constexpr u16 u16Max =  0xFFFF;                //  65'535;
constexpr u32 u32Max =  0xFFFF'FFFF;           //  4'294'967'295;
constexpr u64 u64Max =  0xFFFF'FFFF'FFFF'FFFF; //  18'446'744'073'709'551'615;

// TODO: Change to bit patterns like ints
constexpr r32 r32Min =  1.175494351e-38f;
constexpr r32 r32Max =  3.402823466e+38f;

constexpr r32 r32Epsilon = 31.192092896e-07f;
constexpr r32 r32Pi      = 3.141592654f;

template<typename T>
constexpr inline T
Abs(T value)
{
	return value < 0 ? -value : value;
}

constexpr inline b8
ApproximatelyZero(r32 value)
{
	b8 result = Abs(value) < r32Epsilon;
	return result;
}

inline r32
Cos(r32 theta)
{
	return cosf(theta);
}

inline r64
Cos(r64 theta)
{
	return cos(theta);
}

template<typename T>
constexpr inline T
Clamp(T value, T min, T max)
{
	T sMin = Min(min, max);
	T sMax = Max(min, max);

	T result = value;
	result = Max(result, sMin);
	result = Min(result, sMax);
	return result;
}

template<typename T>
constexpr inline T
Clamp01(T value)
{
	T result = value;
	result = Max(result, (T) 0);
	result = Min(result, (T) 1);
	return result;
}

template<typename T>
constexpr inline b8
IsMultipleOf(T size, T multiple)
{
	T mod = (T) (size % multiple);
	return mod == 0;
}

template<typename T>
constexpr inline T
Lerp(T from, T to, r32 t)
{
	T result = (T) ((1.0f - t)*(r32) from + t*(r32) to);
	return result;
}

template<typename T>
constexpr inline T
LerpClamped(T from, T to, r32 t)
{
	T result = (T) ((1.0f - t)*(r32) from + t*(r32) to);
	result = Clamp(result, from, to);
	return result;
}

template<typename T>
constexpr inline r32
InverseLerp(T from, T to, T value)
{
	Assert((to - from) != 0);
	r32 result = (r32) (value - from) / (r32) (to - from);
	return result;
}

template<typename T>
constexpr inline r32
InverseLerpClamped(T from, T to, T value)
{
	Assert((to - from) != 0);
	r32 result = (r32) (value - from) / (r32) (to - from);
	result = Clamp01(result);
	return result;
}

template<typename T>
constexpr inline T
Max(T lhs, T rhs)
{
	T result = lhs > rhs ? lhs : rhs;
	return result;
}

template<typename T>
constexpr inline T
Min(T lhs, T rhs)
{
	T result = lhs < rhs ? lhs : rhs;
	return result;
}

inline r32
Sin(r32 theta)
{
	return sinf(theta);
}

inline r64
Sin(r64 theta)
{
	return sin(theta);
}

inline r32
Sqrt(r32 theta)
{
	return sqrtf(theta);
}

inline r64
Sqrt(r64 theta)
{
	return sqrt(theta);
}

template<typename T> union v2t;
template<typename T> union v3t;
template<typename T> union v4t;

// -------------------------------------------------------------------------------------------------
// v2

template<typename T>
union v2t
{
	struct
	{
		T x;
		T y;
	};

	// Aliases
	struct
	{
		T yaw;
		T pitch;
	};
	struct
	{
		T pos;
		T size;
	};
	T arr[2];

	constexpr T& operator[] (u32 index);
	constexpr const T& operator[] (u32 index) const;
	template<typename U> constexpr explicit operator v2t<U>();
	template<typename U> constexpr explicit operator v3t<U>();
	template<typename U> constexpr explicit operator v4t<U>();
};

using v2    = v2t<r32>;
using v2r64 = v2t<r64>;
using v2i   = v2t<i32>;
using v2u   = v2t<u32>;
using v2r64 = v2t<r64>;
using v2i8  = v2t<i8>;
using v2i16 = v2t<i16>;
using v2i64 = v2t<i64>;
using v2u8  = v2t<u8>;
using v2u16 = v2t<u16>;
using v2u64 = v2t<u64>;

// Operators
template<typename T>
constexpr inline b8
operator== (v2t<T> lhs, v2t<T> rhs)
{
	b8 result =
		lhs.x == rhs.x &&
		lhs.y == rhs.y;
	return result;
}

template<typename T>
constexpr inline b8
operator!= (v2t<T> lhs, v2t<T> rhs)
{
	b8 result = !(
		lhs.x == rhs.x &&
		lhs.y == rhs.y
	);
	return result;
}

template<typename T>
constexpr inline v2t<T>
operator+ (v2t<T> v)
{
	v2t<T> result = {
		+v.x,
		+v.y
	};
	return result;
}

template<typename T>
constexpr inline v2t<T>
operator+ (v2t<T> lhs, v2t<T> rhs)
{
	v2t<T> result = {
		(T) (lhs.x + rhs.x),
		(T) (lhs.y + rhs.y)
	};
	return result;
}

template<typename T>
constexpr inline void
operator+= (v2t<T>& lhs, v2t<T> rhs)
{
	lhs = lhs + rhs;
}

template<typename T>
constexpr inline v2t<T>
operator- (v2t<T> v)
{
	v2t<T> result = {
		-v.x,
		-v.y
	};
	return result;
}

template<typename T>
constexpr inline v2t<T>
operator- (v2t<T> lhs, v2t<T> rhs)
{
	v2t<T> result = {
		(T) (lhs.x - rhs.x),
		(T) (lhs.y - rhs.y)
	};
	return result;
}

template<typename T>
constexpr inline void
operator-= (v2t<T>& lhs, v2t<T> rhs)
{
	lhs = lhs - rhs;
}

template<typename T>
constexpr inline v2t<T>
operator* (T lhs, v2t<T> rhs)
{
	v2t<T> result = {
		(T) (lhs * rhs.x),
		(T) (lhs * rhs.y)
	};
	return result;
}

template<typename T>
constexpr inline v2t<T>
operator* (v2t<T> lhs, T rhs)
{
	v2t<T> result = {
		(T) (lhs.x * rhs),
		(T) (lhs.y * rhs)
	};
	return result;
}

template<typename T>
constexpr inline v2t<T>
operator* (v2t<T> lhs, v2t<T> rhs)
{
	v2t<T> result = {
		(T) (lhs.x * rhs.x),
		(T) (lhs.y * rhs.y)
	};
	return result;
}

template<typename T>
constexpr inline void
operator*= (v2t<T>& lhs, T rhs)
{
	lhs = lhs * rhs;
}

template<typename T>
constexpr inline void
operator*= (v2t<T>& lhs, v2t<T> rhs)
{
	lhs = lhs * rhs;
}

template<typename T>
constexpr inline v2t<T>
operator/ (v2t<T> lhs, T rhs)
{
	v2t<T> result = {
		(T) (lhs.x / rhs),
		(T) (lhs.y / rhs)
	};
	return result;
}

template<typename T>
constexpr inline v2t<T>
operator/ (T lhs, v2t<T> rhs)
{
	v2t<T> result = {
		(T) (lhs / rhs.x),
		(T) (lhs / rhs.y)
	};
	return result;
}

template<typename T>
constexpr inline v2t<T>
operator/ (v2t<T> lhs, v2t<T> rhs)
{
	v2t<T> result = {
		(T) (lhs.x / rhs.x),
		(T) (lhs.y / rhs.y)
	};
	return result;
}

template<typename T>
constexpr inline void
operator/= (v2t<T>& lhs, T rhs)
{
	lhs = lhs / rhs;
}

template<typename T>
constexpr inline void
operator/= (v2t<T>& lhs, v2t<T> rhs)
{
	lhs = lhs / rhs;
}

template<typename T>
constexpr inline T&
v2t<T>::operator[] (u32 index)
{
	Assert(index < ArrayLength(arr));
	T& result = arr[index];
	return result;
}

template<typename T>
constexpr inline const T&
v2t<T>::operator[] (u32 index) const
{
	Assert(index < ArrayLength(arr));
	const T& result = arr[index];
	return result;
}

template<typename T>
template<typename U>
constexpr inline
v2t<T>::operator v2t<U>()
{
	v2t<U> result = {
		(U) x,
		(U) y
	};
	return result;
}

template<typename T>
template<typename U>
constexpr inline
v2t<T>::operator v3t<U>()
{
	v3t<U> result = {
		(U) x,
		(U) y,
		0
	};
	return result;
}

template<typename T>
template<typename U>
constexpr inline
v2t<T>::operator v4t<U>()
{
	v4t<U> result = {
		(U) x,
		(U) y,
		0,
		0
	};
	return result;
}

template<typename T>
constexpr inline T
Dot(v2t<T> lhs, v2t<T> rhs)
{
	T result = (T) (
		lhs.x*rhs.x +
		lhs.y*rhs.y
	);
	return result;
}

template<typename T>
constexpr inline v2t<T>
Clamp(v2t<T> v, v2t<T> min, v2t<T> max)
{
	v2t<T> result = {
		Clamp(v.x, min.x, max.x),
		Clamp(v.y, min.y, max.y)
	};
	return result;
}

template<typename T>
constexpr inline v2t<T>
Clamp01(v2t<T> v)
{
	v2t<T> result = {
		Clamp01(v.x),
		Clamp01(v.y)
	};
	return result;
}

template<typename T>
constexpr inline v2t<T>
Lerp(v2t<T> from, v2t<T> to, r32 t)
{
	v2t<T> result = {
		Lerp(from.x, to.x, t),
		Lerp(from.y, to.y, t)
	};
	return result;
}

template<typename T>
constexpr inline v2t<T>
LerpClamped(v2t<T> from, v2t<T> to, r32 t)
{
	v2t<T> result = {
		LerpClamped(from.x, to.x, t),
		LerpClamped(from.y, to.y, t)
	};
	return result;
}

template<typename T>
constexpr inline v2t<r32>
InverseLerp(v2t<T> from, v2t<T> to, v2t<T> value)
{
	v2t<r32> result = {
		InverseLerp(from.x, to.x, value.x),
		InverseLerp(from.y, to.y, value.y)
	};
	return result;
}

template<typename T>
constexpr inline v2t<r32>
InverseLerpClamped(v2t<T> from, v2t<T> to, v2t<T> value)
{
	v2t<r32> result = {
		InverseLerpClamped(from.x, to.x, value.x),
		InverseLerpClamped(from.y, to.y, value.y)
	};
	return result;
}

inline r32
Magnitude(v2t<r32> v)
{
	r32 result = Sqrt(
		v.x * v.x +
		v.y * v.y
	);
	return result;
}

inline r64
Magnitude(v2t<r64> v)
{
	r64 result = Sqrt(
		v.x * v.x +
		v.y * v.y
	);
	return result;
}

inline r32
MagnitudeSq(v2t<r32> v)
{
	r32 result =
		v.x * v.x +
		v.y * v.y;
	return result;
}

inline r64
MagnitudeSq(v2t<r64> v)
{
	r64 result =
		v.x * v.x +
		v.y * v.y;
	return result;
}

template<typename T>
constexpr inline v2t<T>
Max(v2t<T> lhs, v2t<T> rhs)
{
	v2t<T> result = {
		Max(lhs.x, rhs.x),
		Max(lhs.y, rhs.y)
	};
	return result;
}

template<typename T>
constexpr inline v2t<T>
Min(v2t<T> lhs, v2t<T> rhs)
{
	v2t<T> result = {
		Min(lhs.x, rhs.x),
		Min(lhs.y, rhs.y)
	};
	return result;
}

inline v2
Normalize(v2 v)
{
	r32 magnitude = Magnitude(v);
	v2 result = {
		v.x / magnitude,
		v.y / magnitude
	};
	return result;
}

inline v2t<r64>
Normalize(v2t<r64> v)
{
	r64 magnitude = Magnitude(v);
	v2t<r64> result = {
		v.x / magnitude,
		v.y / magnitude
	};
	return result;
}

// -------------------------------------------------------------------------------------------------
// v3

template<typename T>
union v3t
{
	struct
	{
		T x;
		T y;
		T z;
	};

	// Aliases
	struct
	{
		T r;
		T g;
		T b;
	};
	struct
	{
		T radius;
		T theta;
		T phi;
	};
	struct
	{
		T yaw;
		T pitch;
		T roll;
	};
	T arr[3];

	constexpr T& operator[] (u32 index);
	constexpr const T& operator[] (u32 index) const;
	template<typename U> constexpr explicit operator v2t<U>();
	template<typename U> constexpr explicit operator v3t<U>();
	template<typename U> constexpr explicit operator v4t<U>();
};

using v3    = v3t<r32>;
using v3r64 = v3t<r64>;
using v3i   = v3t<i32>;
using v3i8  = v3t<i8>;
using v3i16 = v3t<i16>;
using v3i64 = v3t<i64>;
using v3u   = v3t<u32>;
using v3u8  = v3t<u8>;
using v3u16 = v3t<u16>;
using v3u64 = v3t<u64>;

// Operators
template<typename T>
constexpr inline b8
operator== (v3t<T> lhs, v3t<T> rhs)
{
	b8 result =
		lhs.x == rhs.x &&
		lhs.y == rhs.y &&
		lhs.z == rhs.z;
	return result;
}

template<typename T>
constexpr inline b8
operator!= (v3t<T> lhs, v3t<T> rhs)
{
	b8 result = !(
		lhs.x == rhs.x &&
		lhs.y == rhs.y &&
		lhs.z == rhs.z
	);
	return result;
}

template<typename T>
constexpr inline v3t<T>
operator+ (v3t<T> v)
{
	v3t<T> result = {
		+v.x,
		+v.y,
		+v.z
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
operator+ (v3t<T> lhs, v3t<T> rhs)
{
	v3t<T> result = {
		(T) (lhs.x + rhs.x),
		(T) (lhs.y + rhs.y),
		(T) (lhs.z + rhs.z)
	};
	return result;
}

template<typename T>
constexpr inline void
operator+= (v3t<T>& lhs, v3t<T> rhs)
{
	lhs = lhs + rhs;
}

template<typename T>
constexpr inline v3t<T>
operator- (v3t<T> v)
{
	v3t<T> result = {
		-v.x,
		-v.y,
		-v.z
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
operator- (v3t<T> lhs, v3t<T> rhs)
{
	v3t<T> result = {
		(T) (lhs.x - rhs.x),
		(T) (lhs.y - rhs.y),
		(T) (lhs.z - rhs.z)
	};
	return result;
}

template<typename T>
constexpr inline void
operator-= (v3t<T>& lhs, v3t<T> rhs)
{
	lhs = lhs - rhs;
}

template<typename T>
constexpr inline v3t<T>
operator* (T lhs, v3t<T> rhs)
{
	v3t<T> result = {
		(T) (lhs * rhs.x),
		(T) (lhs * rhs.y),
		(T) (lhs * rhs.z)
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
operator* (v3t<T> lhs, T rhs)
{
	v3t<T> result = {
		(T) (lhs.x * rhs),
		(T) (lhs.y * rhs),
		(T) (lhs.z * rhs)
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
operator* (v3t<T> lhs, v3t<T> rhs)
{
	v3t<T> result = {
		(T) (lhs.x * rhs.x),
		(T) (lhs.y * rhs.y),
		(T) (lhs.z * rhs.z)
	};
	return result;
}

template<typename T>
constexpr inline void
operator*= (v3t<T>& lhs, T rhs)
{
	lhs = lhs * rhs;
}

template<typename T>
constexpr inline void
operator*= (v3t<T>& lhs, v3t<T> rhs)
{
	lhs = lhs * rhs;
}

template<typename T>
constexpr inline v3t<T>
operator/ (v3t<T> lhs, T rhs)
{
	v3t<T> result = {
		(T) (lhs.x / rhs),
		(T) (lhs.y / rhs),
		(T) (lhs.z / rhs)
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
operator/ (T lhs, v3t<T> rhs)
{
	v3t<T> result = {
		(T) (lhs / rhs.x),
		(T) (lhs / rhs.y),
		(T) (lhs / rhs.z)
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
operator/ (v3t<T> lhs, v3t<T> rhs)
{
	v3t<T> result = {
		(T) (lhs.x / rhs.x),
		(T) (lhs.y / rhs.y),
		(T) (lhs.z / rhs.z)
	};
	return result;
}

template<typename T>
constexpr inline void
operator/= (v3t<T>& lhs, T rhs)
{
	lhs = lhs / rhs;
}

template<typename T>
constexpr inline void
operator/= (v3t<T>& lhs, v3t<T> rhs)
{
	lhs = lhs / rhs;
}

template<typename T>
constexpr inline T&
v3t<T>::operator[] (u32 index)
{
	Assert(index < ArrayLength(arr));
	T& result = arr[index];
	return result;
}

template<typename T>
constexpr inline const T&
v3t<T>::operator[] (u32 index) const
{
	Assert(index < ArrayLength(arr));
	const T& result = arr[index];
	return result;
}

template<typename T>
template<typename U>
constexpr inline
v3t<T>::operator v2t<U>()
{
	v2t<U> result = {
		(U) x,
		(U) y
	};
	return result;
}

template<typename T>
template<typename U>
constexpr inline
v3t<T>::operator v3t<U>()
{
	v3t<U> result = {
		(U) x,
		(U) y,
		(U) z
	};
	return result;
}

template<typename T>
template<typename U>
constexpr inline
v3t<T>::operator v4t<U>()
{
	v4t<U> result = {
		(U) x,
		(U) y,
		(U) z,
		0
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
Clamp(v3t<T> v, v3t<T> min, v3t<T> max)
{
	v3t<T> result = {
		Clamp(v.x, min.x, max.x),
		Clamp(v.y, min.y, max.y),
		Clamp(v.z, min.z, max.z)
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
Clamp01(v3t<T> v)
{
	v3t<T> result = {
		Clamp01(v.x),
		Clamp01(v.y),
		Clamp01(v.z)
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
Cross(v3t<T> lhs, v3t<T> rhs)
{
	// TODO: This is right handed. What's our system?
	v3t<T> result = {
		(T) (lhs.y*rhs.z - lhs.z*rhs.y),
		(T) (lhs.z*rhs.x - lhs.x*rhs.z),
		(T) (lhs.x*rhs.y - lhs.y*rhs.x)
	};
	return result;
}

template<typename T>
constexpr inline T
Dot(v3t<T> lhs, v3t<T> rhs)
{
	T result = (T) (
		lhs.x*rhs.x +
		lhs.y*rhs.y +
		lhs.z*rhs.z
	);
	return result;
}

inline v3
GetOrbitPos(v3 target, v2 yp, r32 radius)
{
	// NOTE: Pitch, then yaw

	r32 cy = Cos(-yp.yaw);
	r32 sy = Sin(-yp.yaw);
	r32 cp = Cos(yp.pitch);
	r32 sp = Sin(yp.pitch);

	v3 offset = radius * v3 { sy*cp, -sp, cy*cp };

	v3 pos = target + offset;
	return pos;
}

inline v3t<r64>
GetOrbitPos(v3t<r64> target, v2t<r64> yp, r64 radius)
{
	// NOTE: Pitch, then yaw

	r64 cy = Cos(-yp.yaw);
	r64 sy = Sin(-yp.yaw);
	r64 cp = Cos(yp.pitch);
	r64 sp = Sin(yp.pitch);

	v3t<r64> offset = radius * v3t<r64> { sy*cp, -sp, cy*cp };

	v3t<r64> pos = target + offset;
	return pos;
}

template<typename T>
constexpr inline v3t<T>
Lerp(v3t<T> from, v3t<T> to, r32 t)
{
	v3t<T> result = {
		Lerp(from.x, to.x, t),
		Lerp(from.y, to.y, t),
		Lerp(from.z, to.z, t)
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
LerpClamped(v3t<T> from, v3t<T> to, r32 t)
{
	v3t<T> result = {
		LerpClamped(from.x, to.x, t),
		LerpClamped(from.y, to.y, t),
		LerpClamped(from.z, to.z, t)
	};
	return result;
}

template<typename T>
constexpr inline v3t<r32>
InverseLerp(v3t<T> from, v3t<T> to, v3t<T> value)
{
	v3t<r32> result = {
		InverseLerp(from.x, to.x, value.x),
		InverseLerp(from.y, to.y, value.y),
		InverseLerp(from.z, to.z, value.z)
	};
	return result;
}

template<typename T>
constexpr inline v3t<r32>
InverseLerpClamped(v3t<T> from, v3t<T> to, v3t<T> value)
{
	v3t<r32> result = {
		InverseLerpClamped(from.x, to.x, value.x),
		InverseLerpClamped(from.y, to.y, value.y),
		InverseLerpClamped(from.z, to.z, value.z)
	};
	return result;
}

inline r32
Magnitude(v3t<r32> v)
{
	r32 result = Sqrt(
		v.x * v.x +
		v.y * v.y +
		v.z * v.z
	);
	return result;
}

inline r64
Magnitude(v3t<r64> v)
{
	r64 result = Sqrt(
		v.x * v.x +
		v.y * v.y +
		v.z * v.z
	);
	return result;
}

inline r32
MagnitudeSq(v3t<r32> v)
{
	r32 result =
		v.x * v.x +
		v.y * v.y +
		v.z * v.z;
	return result;
}

inline r64
MagnitudeSq(v3t<r64> v)
{
	r64 result =
		v.x * v.x +
		v.y * v.y +
		v.z * v.z;
	return result;
}

template<typename T>
constexpr inline v3t<T>
Max(v3t<T> lhs, v3t<T> rhs)
{
	v3t<T> result = {
		Max(lhs.x, rhs.x),
		Max(lhs.y, rhs.y),
		Max(lhs.z, rhs.z)
	};
	return result;
}

template<typename T>
constexpr inline v3t<T>
Min(v3t<T> lhs, v3t<T> rhs)
{
	v3t<T> result = {
		Min(lhs.x, rhs.x),
		Min(lhs.y, rhs.y),
		Min(lhs.z, rhs.z)
	};
	return result;
}

inline v3
Normalize(v3 v)
{
	r32 magnitude = Magnitude(v);
	v3 result = {
		v.x / magnitude,
		v.y / magnitude,
		v.z / magnitude
	};
	return result;
}

inline v3t<r64>
Normalize(v3t<r64> v)
{
	r64 magnitude = Magnitude(v);
	v3t<r64> result = {
		v.x / magnitude,
		v.y / magnitude,
		v.z / magnitude
	};
	return result;
}

// -------------------------------------------------------------------------------------------------
// v4

template<typename T>
union v4t
{
	struct
	{
		T x;
		T y;
		T z;
		T w;
	};

	// Aliases
	struct
	{
		T r;
		T g;
		T b;
		T a;
	};
	struct
	{
		v2t<T> pos;
		v2t<T> size;
	};
	T arr[4];

	constexpr T& operator[] (u32 index);
	constexpr const T& operator[] (u32 index) const;
	template<typename U> constexpr explicit operator v2t<U>();
	template<typename U> constexpr explicit operator v3t<U>();
	template<typename U> constexpr explicit operator v4t<U>();
};

using v4    = v4t<r32>;
using v4r64 = v4t<r64>;
using v4i   = v4t<i32>;
using v4i8  = v4t<i8>;
using v416  = v4t<i16>;
using v4i64 = v4t<i64>;
using v4u   = v4t<u32>;
using v4u8  = v4t<u8>;
using v4u16 = v4t<u16>;
using v4u64 = v4t<u64>;

// Operators
template<typename T>
constexpr inline b8
operator== (v4t<T> lhs, v4t<T> rhs)
{
	b8 result =
		lhs.x == rhs.x &&
		lhs.y == rhs.y &&
		lhs.z == rhs.z &&
		lhs.w == rhs.w;
	return result;
}

template<typename T>
constexpr inline b8
operator!= (v4t<T> lhs, v4t<T> rhs)
{
	b8 result = !(
		lhs.x == rhs.x &&
		lhs.y == rhs.y &&
		lhs.z == rhs.z &&
		lhs.w == rhs.w
	);
	return result;
}

template<typename T>
constexpr inline v4t<T>
operator+ (v4t<T> v)
{
	v4t<T> result = {
		+v.x,
		+v.y,
		+v.z,
		+v.w
	};
	return result;
}

template<typename T>
constexpr inline v4t<T>
operator+ (v4t<T> lhs, v4t<T> rhs)
{
	v4t<T> result = {
		(T) (lhs.x + rhs.x),
		(T) (lhs.y + rhs.y),
		(T) (lhs.z + rhs.z),
		(T) (lhs.w + rhs.w)
	};
	return result;
}

template<typename T>
constexpr inline void
operator+= (v4t<T>& lhs, v4t<T> rhs)
{
	lhs = lhs + rhs;
}

template<typename T>
constexpr inline v4t<T>
operator- (v4t<T> v)
{
	v4t<T> result = {
		-v.x,
		-v.y,
		-v.z,
		-v.w
	};
	return result;
}

template<typename T>
constexpr inline v4t<T>
operator- (v4t<T> lhs, v4t<T> rhs)
{
	v4t<T> result = {
		(T) (lhs.x - rhs.x),
		(T) (lhs.y - rhs.y),
		(T) (lhs.z - rhs.z),
		(T) (lhs.w - rhs.w)
	};
	return result;
}

template<typename T>
constexpr inline void
operator-= (v4t<T>& lhs, v4t<T> rhs)
{
	lhs = lhs - rhs;
}

template<typename T>
constexpr inline v4t<T>
operator* (T lhs, v4t<T> rhs)
{
	v4t<T> result = {
		(T) (lhs * rhs.x),
		(T) (lhs * rhs.y),
		(T) (lhs * rhs.z),
		(T) (lhs * rhs.w)
	};
	return result;
}

template<typename T>
constexpr inline v4t<T>
operator* (v4t<T> lhs, T rhs)
{
	v4t<T> result = {
		(T) (lhs.x * rhs),
		(T) (lhs.y * rhs),
		(T) (lhs.z * rhs),
		(T) (lhs.w * rhs)
	};
	return result;
}

template<typename T>
constexpr inline v4t<T>
operator* (v4t<T> lhs, v4t<T> rhs)
{
	v4t<T> result = {
		(T) (lhs.x * rhs.x),
		(T) (lhs.y * rhs.y),
		(T) (lhs.z * rhs.z),
		(T) (lhs.w * rhs.w)
	};
	return result;
}

template<typename T>
constexpr inline void
operator*= (v4t<T>& lhs, T rhs)
{
	lhs = lhs * rhs;
}

template<typename T>
constexpr inline void
operator*= (v4t<T>& lhs, v4t<T> rhs)
{
	lhs = lhs * rhs;
}

template<typename T>
constexpr inline v4t<T>
operator/ (v4t<T> lhs, T rhs)
{
	v4t<T> result = {
		(T) (lhs.x / rhs),
		(T) (lhs.y / rhs),
		(T) (lhs.z / rhs),
		(T) (lhs.w / rhs)
	};
	return result;
}

template<typename T>
constexpr inline v4t<T>
operator/ (T lhs, v4t<T> rhs)
{
	v4t<T> result = {
		(T) (lhs / rhs.x),
		(T) (lhs / rhs.y),
		(T) (lhs / rhs.z),
		(T) (lhs / rhs.w)
	};
	return result;
}

template<typename T>
constexpr inline v4t<T>
operator/ (v4t<T> lhs, v4t<T> rhs)
{
	v4t<T> result = {
		(T) (lhs.x / rhs.x),
		(T) (lhs.y / rhs.y),
		(T) (lhs.z / rhs.z),
		(T) (lhs.w / rhs.w)
	};
	return result;
}

template<typename T>
constexpr inline void
operator/= (v4t<T>& lhs, T rhs)
{
	lhs = lhs / rhs;
}

template<typename T>
constexpr inline void
operator/= (v4t<T>& lhs, v4t<T> rhs)
{
	lhs = lhs / rhs;
}

template<typename T>
constexpr inline T&
v4t<T>::operator[] (u32 index)
{
	Assert(index < ArrayLength(arr));
	T& result = arr[index];
	return result;
}

template<typename T>
constexpr inline const T&
v4t<T>::operator[] (u32 index) const
{
	Assert(index < ArrayLength(arr));
	const T& result = arr[index];
	return result;
}

template<typename T>
template<typename U>
constexpr inline
v4t<T>::operator v2t<U>()
{
	v2t<U> result = {
		(U) x,
		(U) y
	};
	return result;
}

template<typename T>
template<typename U>
constexpr inline
v4t<T>::operator v3t<U>()
{
	v3t<U> result = {
		(U) x,
		(U) y,
		(U) z
	};
	return result;
}

template<typename T>
template<typename U>
constexpr inline
v4t<T>::operator v4t<U>()
{
	v4t<U> result = {
		(U) x,
		(U) y,
		(U) z,
		(U) w
	};
	return result;
}

template<typename T>
constexpr inline v4t<T>
Clamp(v4t<T> v, v4t<T> min, v4t<T> max)
{
	v4t<T> result = {
		Clamp(v.x, min.x, max.x),
		Clamp(v.y, min.y, max.y),
		Clamp(v.z, min.z, max.z),
		Clamp(v.w, min.w, max.w)
	};
	return result;
}

template<typename T>
constexpr inline v4t<T>
Clamp01(v4t<T> v)
{
	v4t<T> result = {
		Clamp01(v.x),
		Clamp01(v.y),
		Clamp01(v.z),
		Clamp01(v.w)
	};
	return result;
}

template<typename T>
constexpr inline T
Dot(v4t<T> lhs, v4t<T> rhs)
{
	T result = (T) (
		lhs.x*rhs.x +
		lhs.y*rhs.y +
		lhs.z*rhs.z +
		lhs.w*rhs.w
	);
	return result;
}

template<typename T>
constexpr inline v4t<T>
Lerp(v4t<T> from, v4t<T> to, r32 t)
{
	v4t<T> result = {
		Lerp(from.x, to.x, t),
		Lerp(from.y, to.y, t),
		Lerp(from.z, to.z, t),
		Lerp(from.w, to.w, t)
	};
	return result;
}

template<typename T>
constexpr inline v4t<T>
LerpClamped(v4t<T> from, v4t<T> to, r32 t)
{
	v4t<T> result = {
		LerpClamped(from.x, to.x, t),
		LerpClamped(from.y, to.y, t),
		LerpClamped(from.z, to.z, t),
		LerpClamped(from.w, to.w, t)
	};
	return result;
}

template<typename T>
constexpr inline v4t<r32>
InverseLerp(v4t<T> from, v4t<T> to, v4t<T> value)
{
	v4t<r32> result = {
		InverseLerp(from.x, to.x, value.x),
		InverseLerp(from.y, to.y, value.y),
		InverseLerp(from.z, to.z, value.z),
		InverseLerp(from.w, to.w, value.w)
	};
	return result;
}

template<typename T>
constexpr inline v4t<r32>
InverseLerpClamped(v4t<T> from, v4t<T> to, v4t<T> value)
{
	v4t<r32> result = {
		InverseLerpClamped(from.x, to.x, value.x),
		InverseLerpClamped(from.y, to.y, value.y),
		InverseLerpClamped(from.z, to.z, value.z),
		InverseLerpClamped(from.w, to.w, value.w)
	};
	return result;
}

inline r32
Magnitude(v4t<r32> v)
{
	r32 result = Sqrt(
		v.x * v.x +
		v.y * v.y +
		v.z * v.z +
		v.w * v.w
	);
	return result;
}

inline r64
Magnitude(v4t<r64> v)
{
	r64 result = Sqrt(
		v.x * v.x +
		v.y * v.y +
		v.z * v.z +
		v.w * v.w
	);
	return result;
}

inline r32
MagnitudeSq(v4t<r32> v)
{
	r32 result =
		v.x * v.x +
		v.y * v.y +
		v.z * v.z +
		v.w * v.w;
	return result;
}

inline r64
MagnitudeSq(v4t<r64> v)
{
	r64 result =
		v.x * v.x +
		v.y * v.y +
		v.z * v.z +
		v.w * v.w;
	return result;
}

template<typename T>
constexpr inline v4t<T>
Max(v4t<T> lhs, v4t<T> rhs)
{
	v4t<T> result = {
		Max(lhs.x, rhs.x),
		Max(lhs.y, rhs.y),
		Max(lhs.z, rhs.z),
		Max(lhs.w, rhs.w)
	};
	return result;
}

template<typename T>
constexpr inline v4t<T>
Min(v4t<T> lhs, v4t<T> rhs)
{
	v4t<T> result = {
		Min(lhs.x, rhs.x),
		Min(lhs.y, rhs.y),
		Min(lhs.z, rhs.z),
		Min(lhs.w, rhs.w)
	};
	return result;
}

inline v4
Normalize(v4 v)
{
	r32 magnitude = Magnitude(v);
	v4 result = {
		v.x / magnitude,
		v.y / magnitude,
		v.z / magnitude,
		v.w / magnitude
	};
	return result;
}

inline v4t<r64>
Normalize(v4t<r64> v)
{
	r64 magnitude = Magnitude(v);
	v4t<r64> result = {
		v.x / magnitude,
		v.y / magnitude,
		v.z / magnitude,
		v.w / magnitude
	};
	return result;
}

// -------------------------------------------------------------------------------------------------
// Matrix

union Matrix
{
	struct
	{
		r32 m00, m10, m20, m30;
		r32 m01, m11, m21, m31;
		r32 m02, m12, m22, m32;
		r32 m03, m13, m23, m33;
	};

	// Aliases
	struct
	{
		// TODO: Not sure if this is correct
		r32 xx,  yx,  zx,  tx;
		r32 xy,  yy,  zy,  ty;
		r32 xz,  yz,  zz,  tz;
		r32 m03, m13, m23, m33;
	};
	struct
	{
		r32 sx,  m10, m20, m30;
		r32 m01, sy,  m21, m31;
		r32 m02, m12, sz,  m32;
		r32 m03, m13, m23, m33;
	};
	r32 raw[16];
	r32 arr[4][4];
	v4 col[4];

	constexpr v4& operator[] (u32 _col);
	constexpr const v4& operator[] (u32 _col) const;
};

constexpr inline v4
Row(const Matrix& m, u32 row)
{
	v4 result = {
		m[0][row],
		m[1][row],
		m[2][row],
		m[3][row]
	};
	return result;
}

constexpr inline const v4&
Col(const Matrix& m, u32 col)
{
	const v4& result = m.col[col];
	return result;
}

constexpr inline Matrix
operator* (const Matrix& lhs, const Matrix& rhs)
{
	Matrix result = {
		Dot(Row(lhs, 0), Col(rhs, 0)), Dot(Row(lhs, 1), Col(rhs, 0)), Dot(Row(lhs, 2), Col(rhs, 0)), Dot(Row(lhs, 3), Col(rhs, 0)),
		Dot(Row(lhs, 0), Col(rhs, 1)), Dot(Row(lhs, 1), Col(rhs, 1)), Dot(Row(lhs, 2), Col(rhs, 1)), Dot(Row(lhs, 3), Col(rhs, 1)),
		Dot(Row(lhs, 0), Col(rhs, 2)), Dot(Row(lhs, 1), Col(rhs, 2)), Dot(Row(lhs, 2), Col(rhs, 2)), Dot(Row(lhs, 3), Col(rhs, 2)),
		Dot(Row(lhs, 0), Col(rhs, 3)), Dot(Row(lhs, 1), Col(rhs, 3)), Dot(Row(lhs, 2), Col(rhs, 3)), Dot(Row(lhs, 3), Col(rhs, 3))
	};
	return result;
}

constexpr inline void
operator*= (Matrix& lhs, const Matrix& rhs)
{
	lhs = {
		Dot(Row(lhs, 0), Col(rhs, 0)), Dot(Row(lhs, 1), Col(rhs, 0)), Dot(Row(lhs, 2), Col(rhs, 0)), Dot(Row(lhs, 3), Col(rhs, 0)),
		Dot(Row(lhs, 0), Col(rhs, 1)), Dot(Row(lhs, 1), Col(rhs, 1)), Dot(Row(lhs, 2), Col(rhs, 1)), Dot(Row(lhs, 3), Col(rhs, 1)),
		Dot(Row(lhs, 0), Col(rhs, 2)), Dot(Row(lhs, 1), Col(rhs, 2)), Dot(Row(lhs, 2), Col(rhs, 2)), Dot(Row(lhs, 3), Col(rhs, 2)),
		Dot(Row(lhs, 0), Col(rhs, 3)), Dot(Row(lhs, 1), Col(rhs, 3)), Dot(Row(lhs, 2), Col(rhs, 3)), Dot(Row(lhs, 3), Col(rhs, 3))
	};
}

// TODO: Versions for v3?
constexpr inline v4
operator* (v4 lhs, const Matrix& rhs)
{
	v4 result = {
		Dot(lhs, rhs.col[0]),
		Dot(lhs, rhs.col[1]),
		Dot(lhs, rhs.col[2]),
		Dot(lhs, rhs.col[3])
	};
	return result;
}

constexpr inline void
operator*= (v4& lhs, const Matrix& rhs)
{
	lhs = lhs * rhs;
}

constexpr inline v4&
Matrix::operator[] (u32 _col)
{
	Assert(_col < ArrayLength(col));
	v4& result = col[_col];
	return result;
}

constexpr inline const v4&
Matrix::operator[] (u32 _col) const
{
	Assert(_col < ArrayLength(arr));
	const v4& result = col[_col];
	return result;
}

constexpr inline Matrix
Identity()
{
	Matrix result = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	return result;
}

// TODO: What about scale?
constexpr inline Matrix
InvertRT(const Matrix& m)
{
	Matrix result = {
		m.xx, m.xy, m.xz, -Dot(Row(m, 0), Row(m, 3)),
		m.yx, m.yy, m.yz, -Dot(Row(m, 1), Row(m, 3)),
		m.zx, m.zy, m.zz, -Dot(Row(m, 2), Row(m, 3)),
		0.0f, 0.0f, 0.0f, 1.0f,
	};
	return result;
}

inline Matrix
LookAt(v3 pos, v3 target)
{
	v3 u = { 0.0f, 1.0f, 0.0f };
	v3 z = Normalize(pos - target);
	v3 x = Normalize(Cross(u, z));
	v3 y = Normalize(Cross(z, x));

	Matrix result = {
		x.x,  x.y,  x.z,  -Dot(pos, x),
		y.x,  y.y,  y.z,  -Dot(pos, y),
		z.x,  z.y,  z.z,  -Dot(pos, z),
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return result;
}

inline Matrix
Orbit(v3 target, v2 yp, r32 radius)
{
	// NOTE:Pitch, then yaw

	r32 cy = Cos(-yp.yaw);
	r32 sy = Sin(-yp.yaw);
	r32 cp = Cos(yp.pitch);
	r32 sp = Sin(yp.pitch);

	v3 offset = radius * v3 { sy*cp, -sp, cy*cp };

	v3 pos = target + offset;
	return LookAt(pos, target);
}

constexpr inline Matrix
Orthographic(v2 size, r32 near, r32 far)
{
	Matrix result = {};
	result[0][0] = 2.0f / size.x;
	result[1][1] = 2.0f / size.y;
	result[2][2] = 1.0f / (near - far);
	result[2][3] = near / (near - far);
	result[3][3] = 1.0f;
	return result;
}

constexpr inline Matrix
Transpose(const Matrix& m)
{
	Matrix result = {
		m.m00, m.m01, m.m02, m.m03,
		m.m10, m.m11, m.m12, m.m13,
		m.m20, m.m21, m.m22, m.m23,
		m.m30, m.m31, m.m32, m.m33,
	};
	return result;
}

constexpr inline void
SetPosition(Matrix& m, v2 pos)
{
	m.tx = pos.x;
	m.ty = pos.y;
}

constexpr inline void
SetPosition(Matrix& m, v2 pos, r32 zPos)
{
	m.tx = pos.x;
	m.ty = pos.y;
	m.tz = zPos;
}

constexpr inline void
SetPosition(Matrix& m, v3 pos)
{
	m.tx = pos.x;
	m.ty = pos.y;
	m.tz = pos.z;
}

constexpr inline void
SetTranslation(Matrix& m, v2 pos)
{
	SetPosition(m, pos);
}

constexpr inline void
SetTranslation(Matrix& m, v2 pos, r32 zPos)
{
	SetPosition(m, pos, zPos);
}

constexpr inline void
SetTranslation(Matrix& m, v3 pos)
{
	SetPosition(m, pos);
}

inline void
SetRotation(Matrix& m, v2 yp)
{
	r32 cy = Cos(yp.yaw);
	r32 sy = Sin(yp.yaw);
	r32 cp = Cos(-yp.pitch);
	r32 sp = Sin(-yp.pitch);

	m.m00 = cy;
	m.m10 = sy*sp;
	m.m20 = sy*cp;
	m.m01 = 0;
	m.m11 = cp;
	m.m21 = -sp;
	m.m02 = -sy;
	m.m12 = cy*sp;
	m.m22 = cy*cp;
}

inline void
SetRotation(Matrix& m, v3 ypr)
{
	// NOTE: Conventions
	// Tait-Bryan
	// Yaw (y), then pitch (x), then roll (z)
	// Active, intrinsic rotation
	// Positive yaw rotates right
	// Positive pitch rotates up
	// Positive rolls rotates right

	r32 cy = Cos(ypr.yaw);
	r32 sy = Sin(ypr.yaw);
	r32 cp = Cos(-ypr.pitch);
	r32 sp = Sin(-ypr.pitch);
	r32 cr = Cos(ypr.roll);
	r32 sr = Sin(ypr.roll);

	m.m00 = cr*cy;
	m.m10 = cr*sy*sp - sr*cp;
	m.m20 = cr*sy*cp + sr*sp;
	m.m01 = sr*cy;
	m.m11 = sr*sy*sp + cr*cp;
	m.m21 = sr*sy*cp - cr*sp;
	m.m02 = -sy;
	m.m12 = cy*sp;
	m.m22 = cy*cp;
}

constexpr inline void
SetScale(Matrix& m, v2 scale)
{
	m.sx = scale.x;
	m.sy = scale.y;
}

constexpr inline void
SetScale(Matrix& m, v2 scale, r32 zScale)
{
	m.sx = scale.x;
	m.sy = scale.y;
	m.sz = zScale;
}

constexpr inline void
SetScale(Matrix& m, v3 scale)
{
	m.sx = scale.x;
	m.sy = scale.y;
	m.sz = scale.z;
}

inline void
SetSRT(Matrix& m, v3 scale, v3 ypr, v3 pos)
{
	m.tx = pos.x;
	m.ty = pos.y;
	m.tz = pos.z;

	SetRotation(m, ypr);

	m.xx *= scale.x;
	m.xy *= scale.x;
	m.xz *= scale.x;

	m.yx *= scale.y;
	m.yy *= scale.y;
	m.yz *= scale.y;

	m.zx *= scale.z;
	m.zy *= scale.z;
	m.zz *= scale.z;
}

inline void
SetSRT(Matrix& m, v2 scale, v2 yp, v2 pos)
{
	m.tx = pos.x;
	m.ty = pos.y;

	SetRotation(m, yp);

	m.xx *= scale.x;
	m.xy *= scale.x;
	m.xz *= scale.x;

	m.yx *= scale.y;
	m.yy *= scale.y;
	m.yz *= scale.y;
}

inline Matrix
GetSRT(v3 scale, v3 ypr, v3 pos)
{
	Matrix result = Identity();
	SetSRT(result, scale, ypr, pos);
	return result;
}

inline Matrix
GetSRT(v2 scale, v2 yp, v2 pos)
{
	Matrix result = Identity();
	SetSRT(result, scale, yp, pos);
	return result;
}

inline void
SetSR(Matrix& m, v3 scale, v3 ypr)
{
	SetRotation(m, ypr);

	m.xx *= scale.x;
	m.xy *= scale.x;
	m.xz *= scale.x;

	m.yx *= scale.y;
	m.yy *= scale.y;
	m.yz *= scale.y;

	m.zx *= scale.z;
	m.zy *= scale.z;
	m.zz *= scale.z;
}

inline void
SetSR(Matrix& m, v2 scale, v2 yp)
{
	SetRotation(m, yp);

	m.xx *= scale.x;
	m.xy *= scale.x;
	m.xz *= scale.x;

	m.yx *= scale.y;
	m.yy *= scale.y;
	m.yz *= scale.y;
}

inline Matrix
GetSR(v3 scale, v3 ypr)
{
	Matrix result = Identity();
	SetSR(result, scale, ypr);
	return result;
}

inline Matrix
GetSR(v2 scale, v2 yp)
{
	Matrix result = Identity();
	SetSR(result, scale, yp);
	return result;
}

constexpr inline void
SetST(Matrix& m, v3 scale, v3 pos)
{
	m.tx = pos.x;
	m.ty = pos.y;
	m.tz = pos.z;

	m.xx = scale.x;
	m.xy = 0.0f;
	m.xz = 0.0f;

	m.yx = 0.0f;
	m.yy = scale.y;
	m.yz = 0.0f;

	m.zx = 0.0f;
	m.zy = 0.0f;
	m.zz = scale.z;
}

constexpr inline void
SetST(Matrix& m, v2 scale, v2 pos)
{
	m.tx = pos.x;
	m.ty = pos.y;

	m.xx = scale.x;
	m.xy = 0.0f;
	m.xz = 0.0f;

	m.yx = 0.0f;
	m.yy = scale.y;
	m.yz = 0.0f;

	m.zx = 0.0f;
	m.zy = 1.0f;
	m.zz = 0.0f;
}

constexpr inline Matrix
GetST(v3 scale, v3 pos)
{
	Matrix result = Identity();
	SetST(result, scale, pos);
	return result;
}

constexpr inline Matrix
GetST(v2 scale, v2 pos)
{
	Matrix result = Identity();
	SetST(result, scale, pos);
	return result;
}

inline void
SetRT(Matrix& m, v3 ypr, v3 pos)
{
	SetRotation(m, ypr);

	m.tx = pos.x;
	m.ty = pos.y;
	m.tz = pos.z;
}

inline void
SetRT(Matrix& m, v2 yp, v2 pos)
{
	SetRotation(m, yp);

	m.tx = pos.x;
	m.ty = pos.y;
}

inline Matrix
GetRT(v3 ypr, v3 pos)
{
	Matrix result = Identity();
	SetRT(result, ypr, pos);
	return result;
}

inline Matrix
GetRT(v2 yp, v2 pos)
{
	Matrix result = Identity();
	SetRT(result, yp, pos);
	return result;
}

// -------------------------------------------------------------------------------------------------
// Rect

template<typename T>
inline v4t<T>
ClampRect(v4t<T> rect, v4t<T> bounds)
{
	v2t<T> min = bounds.pos              + Min(v2t<T>{ 0, 0 }, bounds.size - rect.size);
	v2t<T> max = bounds.size - rect.size - Min(v2t<T>{ 0, 0 }, bounds.size - rect.size);

	v4t<T> result = {};
	result.pos  = Clamp(rect.pos, min, max);
	result.size = rect.size;
	return result;
}

template<typename T>
inline v4t<T>
GrowRect(v4t<T> rect, T radius)
{
	v4t<T> result = {};
	result.pos  = rect.pos  - v2t<T>{ (T) (1 * radius), (T) (1 * radius) };
	result.size = rect.size + v2t<T>{ (T) (2 * radius), (T) (2 * radius) };
	return result;
}

template<typename T>
inline v4t<T>
GrowRect(v4t<T> rect, v2t<T> radii)
{
	v4t<T> result = {};
	result.pos  = rect.pos  - v2t<T>{ (T) (1 * radii.x), (T) (1 * radii.y) };
	result.size = rect.size + v2t<T>{ (T) (2 * radii.x), (T) (2 * radii.y) };
	return result;
}

template<typename T>
inline v4t<T>
GrowRect(v4t<T> rect, v4t<T> radii)
{
	v4t<T> result = {};
	result.pos  = rect.pos  - v2t<T>{ radii.x, radii.y };
	result.size = rect.size + v2t<T>{ radii.z, radii.w };
	return result;
}

template<typename T>
inline v4t<T>
MakeRect(v2t<T> center, T radius)
{
	v4t<T> result = {};
	result.pos  = center - v2t<T>{ radius, radius };
	result.size =  (T) 2 * v2t<T>{ radius, radius };
	return result;
}

template<typename T>
inline v4t<T>
MakeRect(v2t<T> center, v2t<T> radii)
{
	v4t<T> result = {};
	result.pos  = center - radii;
	result.size =  (T) 2 * radii;
	return result;
}

template<typename T>
inline v4t<T>
MakeRect(v2t<T> center, v4t<T> radii)
{
	v4t<T> result = {};
	result.pos  = center - v2t<T>{ radii.x, radii.y };
	result.size = { (T) (radii.x + radii.z), (T) (radii.y + radii.w) };
	return result;
}

template<typename T>
inline b8
RectContains(v4t<T> rect, v2t<T> pos)
{
	b8 result = true;
	result &= pos.x >= rect.pos.x;
	result &= pos.x <  rect.pos.x + rect.size.x;
	result &= pos.y >= rect.pos.y;
	result &= pos.y <  rect.pos.y + rect.size.y;
	return result;
}

template<typename T>
inline v4t<T>
RectCombine(v4t<T> lhs, v4t<T> rhs)
{
	v2t<T> lMax = lhs.pos + lhs.size;
	v2t<T> rMax = rhs.pos + rhs.size;
	v2t<T> max = Max(lMax, rMax);

	v4t<T> result = {};
	result.pos  = Min(lhs.pos, rhs.pos);
	result.size = max - result.pos;
	return result;
}

// -------------------------------------------------------------------------------------------------
// Color

constexpr inline u16
Color16(u8 r, u8 g, u8 b)
{
	// |<- MSB
	// RRRR'RGGG'GGGB'BBBB
	u16 rp = (u16) ((r & 0b1111'1000) << 8);
	u16 gp = (u16) ((g & 0b1111'1100) << 3);
	u16 bp = (u16) ((b & 0b1111'1000) >> 3);
	u16 result = (u16) (rp | gp | bp);
	return result;
}

constexpr inline u32
Color32(u8 r, u8 g, u8 b, u8 a)
{
	u32 rp = (u32) (r << 24);
	u32 gp = (u32) (g << 16);
	u32 bp = (u32) (b <<  8);
	u32 ap = (u32) (a <<  0);
	u32 result = rp | gp | bp | ap;
	return result;
}

constexpr inline v4
Color128(u8 r, u8 g, u8 b, u8 a)
{
	// https://developercommunity.visualstudio.com/content/problem/1167562/spurious-warning-c5219-in-1672.html
	v4 result = {
		(r32) r / 255.0f,
		(r32) g / 255.0f,
		(r32) b / 255.0f,
		(r32) a / 255.0f
	};
	return result;
}

namespace Colors16
{
	static const u16 Black   = Color16(0x00, 0x00, 0x00);
	static const u16 Gray    = Color16(0x80, 0x80, 0x80);
	static const u16 White   = Color16(0xFF, 0xFF, 0xFF);
	static const u16 Red     = Color16(0xFF, 0x00, 0x00);
	static const u16 Green   = Color16(0x00, 0xFF, 0x00);
	static const u16 Blue    = Color16(0x00, 0x00, 0xFF);
	static const u16 Cyan    = Color16(0x00, 0xFF, 0xFF);
	static const u16 Magenta = Color16(0xFF, 0x00, 0xFF);
	static const u16 Yellow  = Color16(0xFF, 0xFF, 0x00);
};

namespace Colors128
{
	static const v4 Clear   = Color128(0x00, 0x00, 0x00, 0x00);
	static const v4 Black   = Color128(0x00, 0x00, 0x00, 0xFF);
	static const v4 Gray    = Color128(0x80, 0x80, 0x80, 0xFF);
	static const v4 White   = Color128(0xFF, 0xFF, 0xFF, 0xFF);
	static const v4 Red     = Color128(0xFF, 0x00, 0x00, 0xFF);
	static const v4 Green   = Color128(0x00, 0xFF, 0x00, 0xFF);
	static const v4 Blue    = Color128(0x00, 0x00, 0xFF, 0xFF);
	static const v4 Cyan    = Color128(0x00, 0xFF, 0xFF, 0xFF);
	static const v4 Magenta = Color128(0xFF, 0x00, 0xFF, 0xFF);
	static const v4 Yellow  = Color128(0xFF, 0xFF, 0x00, 0xFF);
};

// -------------------------------------------------------------------------------------------------
// Bit Manipulation

constexpr inline u8
GetByte(u8 index, u32 value) { return (value >> (index * 8)) & 0xFF; }

#define UnpackLSB2(x) GetByte(0, x), GetByte(1, x)
#define UnpackLSB3(x) GetByte(0, x), GetByte(1, x), GetByte(2, x)
#define UnpackLSB4(x) GetByte(0, x), GetByte(1, x), GetByte(2, x), GetByte(3, x)

#define UnpackMSB2(x) GetByte(1, x), GetByte(0, x)
#define UnpackMSB3(x) GetByte(2, x), GetByte(1, x), GetByte(0, x)
#define UnpackMSB4(x) GetByte(3, x), GetByte(2, x), GetByte(1, x), GetByte(0, x)

constexpr inline u8
GetBit(u8 value, u8 index) { return (u8) ((value >> index) & 0x1); }

constexpr inline u8
SetBit(u8 value, u8 bitIndex, u8 bitValue)
{
	value &= ~(1 << bitIndex);
	value |= bitValue << bitIndex;
	return value;
}

#endif
