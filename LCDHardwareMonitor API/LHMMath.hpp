#ifndef LHM_MATH
#define LHM_MATH

// Conventions:
//   Right handed world
//   +X right, +Y up, -Z forward
//   Vectors are column vectors
//   Matrices are applied from the right side of vectors
//   v' = v * M
//   Matrices store axes and translation in rows *CHANGE THIS*
//   Matrices are stored in row-major order
//   Matrices are addressed in row-major order
//   m12 = M[1][2] = y.z
//   Transformations are applied S then R then T


// === General =====================================================================================

const i32 i32Min = -2147483647 - 1;
const i32 i32Max = 2147483647;
const u32 u32Max = 4294967295;
const r32 r32Min = 1.175494351e-38f;
const r32 r32Max = 3.402823466e+38f;

const r32 r32Epsilon = 31.192092896e-07f;
const r32 r32Pi      = 3.141592654f;

template<typename T, typename U, typename V>
inline T
Clamp(T value, U min, V max)
{
	T result = value < min ? min
	         : value > max ? max
	         : value;
	return result;
}

template<typename T, typename U>
inline b32
IsMultipleOf(T size, U multiple)
{
	T mod = size % multiple;
	return mod == 0;
}

template<typename T, typename U>
inline T
Lerp(T lhs, U rhs, r32 t)
{
	T result = (1.0f - t)*lhs + t*rhs;
	return result;
}

template<typename T, typename U>
inline T
Max(T lhs, U rhs)
{
	T result = lhs > rhs ? lhs : rhs;
	return result;
}

template<typename T, typename U>
inline T
Min(T lhs, U rhs)
{
	T result = lhs < rhs ? lhs : rhs;
	return result;
}

template<typename T> union v2t;
template<typename T> union v3t;
template<typename T> union v4t;


// === v2 ==========================================================================================

template<typename T>
union v2t
{
	struct
	{
		T x;
		T y;
	};

	// Aliases
	T arr[2];

	T& operator[] (i32 index);
	template<typename U> explicit operator v2t<U>();
	template<typename U> explicit operator v3t<U>();
	template<typename U> explicit operator v4t<U>();
};

using v2  = v2t<r32>;
using v2r = v2t<r32>;
using v2i = v2t<i32>;
using v2u = v2t<u32>;

// Operators
template<typename T, typename U>
inline b32
operator== (v2t<T> lhs, v2t<U> rhs)
{
	b32 result = lhs.x == rhs.x && lhs.y == rhs.y;
	return result;
}

template<typename T, typename U>
inline b32
operator!= (v2t<T> lhs, v2t<U> rhs)
{
	b32 result = !(lhs.x == rhs.x && lhs.y == rhs.y);
	return result;
}

template<typename T, typename U>
inline v2t<T>
operator+ (v2t<T> lhs, v2t<U> rhs)
{
	v2t<T> result = { lhs.x + rhs.x, lhs.y + rhs.y };
	return result;
}

template<typename T, typename U>
inline void
operator+= (v2t<T>& lhs, U rhs)
{
	lhs = lhs + rhs;
}

template<typename T>
inline v2t<T>
operator- (v2t<T> v)
{
	v2t<T> result = { -v.x, -v.y };
	return result;
}

template<typename T, typename U>
inline v2t<T>
operator- (v2t<T> lhs, v2t<U> rhs)
{
	v2t<T> result = { lhs.x - rhs.x, lhs.y - rhs.y };
	return result;
}

template<typename T, typename U>
inline void
operator-= (v2t<T>& lhs, U rhs)
{
	lhs = lhs - rhs;
}

template<typename T, typename U>
inline v2t<T>
operator* (U multiplier, v2t<T> v)
{
	v2t<T> result = { multiplier * v.x, multiplier * v.y };
	return result;
}

template<typename T, typename U>
inline v2t<T>
operator* (v2t<T> lhs, v2t<U> rhs)
{
	v2t<T> result = { lhs.x * rhs.x, lhs.y * rhs.y };
	return result;
}

template<typename T, typename U>
inline void
operator*= (v2t<T>& lhs, U rhs)
{
	lhs = lhs * rhs;
}

template<typename T, typename U>
inline v2t<T>
operator/ (v2t<T> v, U divisor)
{
	v2t<T> result = { v.x / divisor, v.y / divisor };
	return result;
}

template<typename T, typename U>
inline v2t<T>
operator/ (U dividend, v2t<T> v)
{
	v2t<T> result = { dividend / v.x, dividend / v.y };
	return result;
}

template<typename T, typename U>
inline void
operator/= (v2t<T>& lhs, U rhs)
{
	lhs = lhs / rhs;
}

template<typename T>
inline T&
v2t<T>::operator[] (i32 index)
{
	T& result = arr[index];
	return result;
}

template<typename T>
template<typename U>
inline
v2t<T>::operator v2t<U>()
{
	v2t<U> result = { (U) x, (U) y };
	return result;
}

template<typename T>
template<typename U>
inline
v2t<T>::operator v3t<U>()
{
	v3t<U> result = { (U) x, (U) y, 0 };
	return result;
}

template<typename T>
template<typename U>
inline
v2t<T>::operator v4t<U>()
{
	v4t<U> result = { (U) x, (U) y, 0, 0 };
	return result;
}

template<typename T, typename U>
inline r32
Dot(v2t<T> lhs, v2t<U> rhs)
{
	r32 result = lhs.x*rhs.x + lhs.y*rhs.y;
	return result;
}

template<typename T, typename U, typename V>
inline v2t<T>
Clamp(v2t<T> v, v2t<U> min, v2t<V> max)
{
	v2t<T> result = { Clamp(v.x, min.x, max.x), Clamp(v.y, min.y, max.y) };
	return result;
}

template<typename T, typename U>
inline v2t<T>
Lerp(v2t<T> lhs, v2t<U> rhs, r32 t)
{
	v2t<T> result = { Lerp(lhs.x, rhs.x, t), Lerp(lhs.y, rhs.y, t) };
	return result;
}

template<typename T, typename U>
inline v2t<T>
Max(v2t<T> lhs, v2t<U> rhs)
{
	v2t<T> result = { Max(lhs.x, rhs.x), Max(lhs.y, rhs.y) };
	return result;
}

template<typename T, typename U>
inline v2t<T>
Min(v2t<T> lhs, v2t<U> rhs)
{
	v2t<T> result = { Min(lhs.x, rhs.x), Min(lhs.y, rhs.y) };
	return result;
}

template<typename T>
inline v2t<T>
Normalize(v2t<T> v)
{
	r32 magnitude = sqrt(v.x*v.x + v.y*v.y);
	v2t<T> result = { v.x / magnitude, v.y / magnitude };
	return result;
}


// === v3 ==========================================================================================

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
	T arr[3];

	T& operator[] (i32 index);
	template<typename U> explicit operator v2t<U>();
	template<typename U> explicit operator v3t<U>();
	template<typename U> explicit operator v4t<U>();
};

using v3  = v3t<r32>;
using v3r = v3t<r32>;
using v3i = v3t<i32>;
using v3u = v3t<u32>;

// Operators
template<typename T, typename U>
inline b32
operator== (v3t<T> lhs, v3t<U> rhs)
{
	b32 result = lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
	return result;
}

template<typename T, typename U>
inline b32
operator!= (v3t<T> lhs, v3t<U> rhs)
{
	b32 result = !(lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z);
	return result;
}

template<typename T, typename U>
inline v3t<T>
operator+ (v3t<T> lhs, v3t<U> rhs)
{
	v3t<T> result = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
	return result;
}

template<typename T, typename U>
inline void
operator+= (v3t<T>& lhs, U rhs)
{
	lhs = lhs + rhs;
}

template<typename T>
inline v3t<T>
operator- (v3t<T> v)
{
	v3t<T> result = { -v.x, -v.y, -v.z };
	return result;
}

template<typename T, typename U>
inline v3t<T>
operator- (v3t<T> lhs, v3t<U> rhs)
{
	v3t<T> result = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
	return result;
}

template<typename T, typename U>
inline void
operator-= (v3t<T>& lhs, U rhs)
{
	lhs = lhs - rhs;
}

template<typename T, typename U>
inline v3t<T>
operator* (U multiplier, v3t<T> v)
{
	v3t<T> result = { multiplier * v.x, multiplier * v.y, multiplier * v.z };
	return result;
}

template<typename T, typename U>
inline v3t<T>
operator* (v3t<T> lhs, v3t<U> rhs)
{
	v3t<T> result = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
	return result;
}

template<typename T, typename U>
inline void
operator*= (v3t<T>& lhs, U rhs)
{
	lhs = lhs * rhs;
}

template<typename T, typename U>
inline v3t<T>
operator/ (v3t<T> v, U divisor)
{
	v3t<T> result = { v.x / divisor, v.y / divisor, v.z / divisor };
	return result;
}

template<typename T, typename U>
inline void
operator/= (v3t<T>& lhs, U rhs)
{
	lhs = lhs / rhs;
}

template<typename T, typename U>
inline v3t<T>
operator/ (U dividend, v3t<T> v)
{
	v3t<T> result = { dividend / v.x, dividend / v.y, dividend / v.z };
	return result;
}

template<typename T>
inline T&
v3t<T>::operator[] (i32 index)
{
	T& result = arr[index];
	return result;
}

template<typename T>
template<typename U>
inline
v3t<T>::operator v2t<U>()
{
	v2t<U> result = { (U) x, (U) y };
	return result;
}

template<typename T>
template<typename U>
inline
v3t<T>::operator v3t<U>()
{
	v3t<U> result = { (U) x, (U) y, (U) z };
	return result;
}

template<typename T>
template<typename U>
inline
v3t<T>::operator v4t<U>()
{
	v4t<U> result = { (U) x, (U) y, (U) z, 0 };
	return result;
}

template<typename T, typename U, typename V>
inline v3t<T>
Clamp(v3t<T> v, v3t<U> min, v3t<V> max)
{
	v3t<T> result = { Clamp(v.x, min.x, max.x), Clamp(v.y, min.y, max.y), Clamp(v.z, min.z, max.z) };
	return result;
}

template<typename T, typename U>
inline v3t<T>
Cross(v3t<T> lhs, v3t<U> rhs)
{
	// TODO: This is right handed. What's our system?
	v3t<T> result = {
		lhs.y*rhs.z - lhs.z*rhs.y,
		lhs.z*rhs.x - lhs.x*rhs.z,
		lhs.x*rhs.y - lhs.y*rhs.x
	};
	return result;
}

template<typename T, typename U>
inline r32
Dot(v3t<T> lhs, v3t<U> rhs)
{
	r32 result = lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z;
	return result;
}

template<typename T, typename U>
inline v3t<T>
Lerp(v3t<T> lhs, v3t<U> rhs, r32 t)
{
	v3t<T> result = { Lerp(lhs.x, rhs.x, t), Lerp(lhs.y, rhs.y, t), Lerp(lhs.z, rhs.z, t) };
	return result;
}

template<typename T, typename U>
inline v3t<T>
Max(v3t<T> lhs, v3t<U> rhs)
{
	v3t<T> result = { Max(lhs.x, rhs.x), Max(lhs.y, rhs.y), Max(lhs.z, rhs.z) };
	return result;
}

template<typename T, typename U>
inline v3t<T>
Min(v3t<T> lhs, v3t<U> rhs)
{
	v3t<T> result = { Min(lhs.x, rhs.x), Min(lhs.y, rhs.y), Min(lhs.z, rhs.z) };
	return result;
}

template<typename T>
inline v3t<T>
Normalize(v3t<T> v)
{
	r32 magnitude = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
	v3t<T> result = { v.x / magnitude, v.y / magnitude, v.z / magnitude };
	return result;
}


// === v4 ==========================================================================================

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
	T arr[4];

	T& operator[] (i32 index);
	template<typename U> explicit operator v2t<U>();
	template<typename U> explicit operator v3t<U>();
	template<typename U> explicit operator v4t<U>();
};

using v4  = v4t<r32>;
using v4r = v4t<r32>;
using v4i = v4t<i32>;
using v4u = v4t<u32>;

// Operators
template<typename T, typename U>
inline b32
operator== (v4t<T> lhs, v4t<U> rhs)
{
	b32 result = lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
	return result;
}

template<typename T, typename U>
inline b32
operator!= (v4t<T> lhs, v4t<U> rhs)
{
	v4t<T> result = !(lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w);
	return result;
}

template<typename T, typename U>
inline v4t<T>
operator+ (v4t<T> lhs, v4t<U> rhs)
{
	v4t<T> result = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
	return result;
}

template<typename T, typename U>
inline void
operator+= (v4t<T>& lhs, U rhs)
{
	lhs = lhs + rhs;
}

template<typename T>
inline v4t<T>
operator- (v4t<T> v)
{
	v4t<T> result = { -v.x, -v.y, -v.z, -v.w };
	return result;
}

template<typename T, typename U>
inline v4t<T>
operator- (v4t<T> lhs, v4t<U> rhs)
{
	v4t<T> result = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
	return result;
}

template<typename T, typename U>
inline void
operator-= (v4t<T>& lhs, U rhs)
{
	lhs = lhs - rhs;
}

template<typename T, typename U>
inline v4t<T>
operator* (U multiplier, v4t<T> v)
{
	v4t<T> result = { multiplier * v.x, multiplier * v.y, multiplier * v.z, multiplier * v.w };
	return result;
}

template<typename T, typename U>
inline v4t<T>
operator* (v4t<T> lhs, v4t<U> rhs)
{
	v4t<T> result = { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
	return result;
}

template<typename T, typename U>
inline void
operator*= (v4t<T>& lhs, U rhs)
{
	lhs = lhs * rhs;
}

template<typename T, typename U>
inline v4t<T>
operator/ (v4t<T> v, U divisor)
{
	v4t<T> result = { v.x / divisor, v.y / divisor, v.z / divisor, v.w / divisor };
	return result;
}

template<typename T, typename U>
inline v4t<T>
operator/ (U dividend, v4t<T> v)
{
	v4t<T> result = { dividend / v.x, dividend / v.y, dividend / v.z, dividend / v.w };
	return result;
}

template<typename T, typename U>
inline void
operator/= (v4t<T>& lhs, U rhs)
{
	lhs = lhs / rhs;
}

template<typename T>
inline T&
v4t<T>::operator[] (i32 index)
{
	T& result = arr[index];
	return result;
}

template<typename T>
template<typename U>
inline
v4t<T>::operator v2t<U>()
{
	v2t<U> result = { (U) x, (U) y };
	return result;
}

template<typename T>
template<typename U>
inline
v4t<T>::operator v3t<U>()
{
	v3t<U> result = { (U) x, (U) y, (U) z };
	return result;
}

template<typename T>
template<typename U>
inline
v4t<T>::operator v4t<U>()
{
	v4t<U> result = { (U) x, (U) y, (U) z, (U) w };
	return result;
}

template<typename T, typename U, typename V>
inline v4t<T>
Clamp(v4t<T> v, v4t<U> min, v4t<V> max)
{
	v4t<T> result = {
		Clamp(v.x, min.x, max.x),
		Clamp(v.y, min.y, max.y),
		Clamp(v.z, min.z, max.z),
		Clamp(v.w, min.w, max.w)
	};
	return result;
}

inline v4
Color32(u8 r, u8 g, u8 b, u8 a)
{
	v4 result = {
		r / 255.0f,
		g / 255.0f,
		b / 255.0f,
		a / 255.0f
	};
	return result;
}

template<typename T, typename U>
inline r32
Dot(v4t<T> lhs, v4t<U> rhs)
{
	r32 result = lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z + lhs.w*rhs.w;
	return result;
}

template<typename T, typename U>
inline v4t<T>
Lerp(v4t<T> lhs, v4t<U> rhs, r32 t)
{
	v4t<T> result = {
		Lerp(lhs.x, rhs.x, t),
		Lerp(lhs.y, rhs.y, t),
		Lerp(lhs.z, rhs.z, t),
		Lerp(lhs.w, rhs.w, t)
	};
	return result;
}

template<typename T, typename U>
inline v4t<T>
Max(v4t<T> lhs, v4t<U> rhs)
{
	v4t<T> result = {
		Max(lhs.x, rhs.x),
		Max(lhs.y, rhs.y),
		Max(lhs.z, rhs.z),
		Max(lhs.w, rhs.w)
	};
	return result;
}

template<typename T, typename U>
inline v4t<T>
Min(v4t<T> lhs, v4t<U> rhs)
{
	v4t<T> result = {
		Min(lhs.x, rhs.x),
		Min(lhs.y, rhs.y),
		Min(lhs.z, rhs.z),
		Min(lhs.w, rhs.w)
	};
	return result;
}

template<typename T>
inline v4t<T>
Normalize(v4t<T> v)
{
	r32 magnitude = sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
	v4t<T> result = {
		v.x / magnitude,
		v.y / magnitude,
		v.z / magnitude,
		v.w / magnitude
	};
	return result;
}


// === Matrix ======================================================================================

union Matrix
{
	struct
	{
		r32 m00, m01, m02, m03;
		r32 m10, m11, m12, m13;
		r32 m20, m21, m22, m23;
		r32 m30, m31, m32, m33;
	};

	// Aliases
	struct
	{
		r32 xx, xy, xz, m03;
		r32 yx, yy, yz, m13;
		r32 zx, zy, zz, m23;
		r32 tx, ty, tz, m33;
	};
	struct
	{
		r32 sx,  m01, m02, m03;
		r32 m10, sy,  m12, m13;
		r32 m20, m21, sz,  m23;
		r32 m30, m31, m32, m33;
	};
	r32 raw[16];
	r32 arr[4][4];
	v4 row[4];

	v4& operator[] (i32 row);
};

inline Matrix
Transpose(Matrix m)
{
	Matrix result = {
		m.m00, m.m10, m.m20, m.m30,
		m.m01, m.m11, m.m21, m.m31,
		m.m02, m.m12, m.m22, m.m32,
		m.m03, m.m13, m.m23, m.m33,
	};
	return result;
}

inline Matrix
operator* (Matrix lhs, Matrix rhs)
{
	Matrix rTrans = Transpose(rhs);
	Matrix result = {
		Dot(lhs[0], rTrans[0]), Dot(lhs[0], rTrans[1]), Dot(lhs[0], rTrans[2]), Dot(lhs[0], rTrans[3]),
		Dot(lhs[1], rTrans[0]), Dot(lhs[1], rTrans[1]), Dot(lhs[1], rTrans[2]), Dot(lhs[1], rTrans[3]),
		Dot(lhs[2], rTrans[0]), Dot(lhs[2], rTrans[1]), Dot(lhs[2], rTrans[2]), Dot(lhs[2], rTrans[3]),
		Dot(lhs[3], rTrans[0]), Dot(lhs[3], rTrans[1]), Dot(lhs[3], rTrans[2]), Dot(lhs[3], rTrans[3])
	};
	return result;
}

// TODO: Versions for v3?
template <typename T>
inline v4t<T>
operator* (v4t<T> lhs, Matrix rhs)
{
	// TODO: Get rid of this transpose
	Matrix rTrans = Transpose(rhs);
	v4t<T> result = {
		Dot(lhs, rTrans[0]),
		Dot(lhs, rTrans[1]),
		Dot(lhs, rTrans[2]),
		Dot(lhs, rTrans[3])
	};
	return result;
}

inline v4&
Matrix::operator[] (i32 _row)
{
	v4& result = row[_row];
	return result;
}

inline Matrix
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

template<typename T, typename U>
inline Matrix
LookAt(v3t<T> pos, v3t<U> target)
{
	v3 u = { 0.0f, 1.0f, 0.0f };
	v3 z = Normalize(pos - target);
	v3 x = Normalize(Cross(u, z));
	v3 y = Normalize(Cross(z, x));

	Matrix result = {
		  x.x,   x.y,   x.z, 0.0f,
		  y.x,   y.y,   y.z, 0.0f,
		  z.x,   z.y,   z.z, 0.0f,
		pos.x, pos.y, pos.z, 1.0f
	};
	return result;
}

template<typename T>
inline Matrix
Orthographic(v2t<T> size, r32 near, r32 far)
{
	Matrix result = {};
	result[0][0] =  2.0f / size.x;
	result[1][1] =  2.0f / size.y;
	result[2][2] = -1.0f / (near - far);
	result[3][2] =  near / (near - far);
	result[3][3] =  1.0f;
	return result;
}

template<typename T>
inline void
SetPosition(Matrix& m, v2t<T> pos)
{
	m.tx = pos.x;
	m.ty = pos.y;
}

template<typename T, typename U>
inline void
SetPosition(Matrix& m, v2t<T> pos, U zPos)
{
	m.tx = pos.x;
	m.ty = pos.y;
	m.tz = zPos;
}

template<typename T>
inline void
SetPosition(Matrix& m, v3t<T> pos)
{
	m.tx = pos.x;
	m.ty = pos.y;
	m.tz = pos.z;
}

template<typename T>
inline void
SetScale(Matrix& m, v2t<T> scale)
{
	m.sx = scale.x;
	m.sy = scale.y;
}

template<typename T, typename U>
inline void
SetScale(Matrix& m, v2t<T> scale, U zScale)
{
	m.sx = scale.x;
	m.sy = scale.y;
	m.sz = zScale;
}

template<typename T>
inline void
SetScale(Matrix& m, v3t<T> scale)
{
	m.sx = scale.x;
	m.sy = scale.y;
	m.sz = scale.z;
}
#endif
