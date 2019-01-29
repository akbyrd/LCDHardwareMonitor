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

// TODO: Can't use operators from the VS Watch window if they are
// pass-by-value, but passing by reference requires const everywhere. Ugh.


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
	struct
	{
		T yaw;
		T pitch;
	};
	T arr[2];

	T& operator[] (i32 index);
	const T& operator[] (i32 index) const;
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
operator*= (v2t<T>* lhs, U rhs)
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
inline const T&
v2t<T>::operator[] (i32 index) const
{
	const T& result = arr[index];
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
inline T
Dot(v2t<T> lhs, v2t<U> rhs)
{
	T result = lhs.x*rhs.x + lhs.y*rhs.y;
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
	struct
	{
		T yaw;
		T pitch;
		T roll;
	};
	T arr[3];

	T& operator[] (i32 index);
	const T& operator[] (i32 index) const;
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
inline const T&
v3t<T>::operator[] (i32 index) const
{
	const T& result = arr[index];
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
inline T
Dot(v3t<T> lhs, v3t<U> rhs)
{
	T result = lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z;
	return result;
}

template<typename T, typename U>
inline v3
GetOrbitPos(v3t<T> target, v3t<U> ypr)
{
	// NOTE:
	// Using roll as radius
	// Pitch, then yaw

	r32 cy = cos(-ypr.yaw);
	r32 sy = sin(-ypr.yaw);
	r32 cp = cos(ypr.pitch);
	r32 sp = sin(ypr.pitch);

	v3 offset = ypr.roll * v3 { sy*cp, -sp, cy*cp };

	v3 pos = target + offset;
	return pos;
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
	const T& operator[] (i32 index) const;
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
inline const T&
v4t<T>::operator[] (i32 index) const
{
	const T& result = arr[index];
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
inline T
Dot(v4t<T> lhs, v4t<U> rhs)
{
	T result = lhs.x*rhs.x + lhs.y*rhs.y + lhs.z*rhs.z + lhs.w*rhs.w;
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

	v4& operator[] (i32 col);
	const v4& operator[] (i32 col) const;
};

inline v4
Row(const Matrix& m, i32 row)
{
	v4 result = { m[0][row], m[1][row], m[2][row], m[3][row] };
	return result;
}

inline const v4&
Col(const Matrix& m, i32 col)
{
	const v4& result = m.col[col];
	return result;
}

inline Matrix
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

// TODO: Versions for v3?
template <typename T>
inline v4t<T>
operator* (v4t<T> lhs, const Matrix& rhs)
{
	v4t<T> result = {
		Dot(lhs, rhs.col[0]),
		Dot(lhs, rhs.col[1]),
		Dot(lhs, rhs.col[2]),
		Dot(lhs, rhs.col[3])
	};
	return result;
}

inline v4&
Matrix::operator[] (i32 _col)
{
	v4& result = col[_col];
	return result;
}

inline const v4&
Matrix::operator[] (i32 _col) const
{
	const v4& result = col[_col];
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
		x.x,  x.y,  x.z,  -Dot(pos, x),
		y.x,  y.y,  y.z,  -Dot(pos, y),
		z.x,  z.y,  z.z,  -Dot(pos, z),
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return result;
}

template<typename T, typename U>
inline Matrix
Orbit(v3t<T> target, v3t<U> ypr)
{
	// NOTE:
	// Using roll as radius
	// Pitch, then yaw

	r32 cy = cos(-ypr.yaw);
	r32 sy = sin(-ypr.yaw);
	r32 cp = cos(ypr.pitch);
	r32 sp = sin(ypr.pitch);

	v3 offset = ypr.roll * v3 { sy*cp, -sp, cy*cp };

	v3 pos = target + offset;
	return LookAt(pos, target);
}

template<typename T>
inline Matrix
Orthographic(v2t<T> size, r32 near, r32 far)
{
	Matrix result = {};
	result[0][0] = 2.0f / size.x;
	result[1][1] = 2.0f / size.y;
	result[2][2] = 1.0f / (near - far);
	result[2][3] = near / (near - far);
	result[3][3] = 1.0f;
	return result;
}

inline Matrix
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
SetTranslation(Matrix& m, v2t<T> pos)
{
	SetPosition(m, pos);
}

template<typename T, typename U>
inline void
SetTranslation(Matrix& m, v2t<T> pos, U zPos)
{
	SetPosition(m, pos, zPos);
}

template<typename T>
inline void
SetTranslation(Matrix& m, v3t<T> pos)
{
	SetPosition(m, pos);
}

template<typename T>
inline void
SetRotation(Matrix& m, v2t<T> yp)
{
	r32 cy = cos(yp.yaw);
	r32 sy = sin(yp.yaw);
	r32 cp = cos(-yp.pitch);
	r32 sp = sin(-yp.pitch);

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

template<typename T>
inline void
SetRotation(Matrix& m, v3t<T> ypr)
{
	// NOTE: Conventions
	// Tait-Bryan
	// Yaw (y), then pitch (x), then roll (z)
	// Active, intrinsic rotation
	// Positive yaw rotates right
	// Positive pitch rotates up
	// Positive rolls rotates right

	r32 cy = cos(ypr.yaw);
	r32 sy = sin(ypr.yaw);
	r32 cp = cos(-ypr.pitch);
	r32 sp = sin(-ypr.pitch);
	r32 cr = cos(ypr.roll);
	r32 sr = sin(ypr.roll);

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

template<typename T, typename U, typename V>
inline void
SetSRT(Matrix& m, v3t<T> scale, v3t<U> ypr, v3t<V> pos)
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

template<typename T, typename U, typename V>
inline void
SetSRT(Matrix& m, v2t<T> scale, v2t<U> yp, v2t<V> pos)
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

template<typename T, typename U, typename V>
inline Matrix
GetSRT(v3t<T>& scale, v3t<U> ypr, v3t<V> pos)
{
	Matrix result = Identity();
	SetSRT(result, scale, ypr, pos);
	return result;
}

template<typename T, typename U, typename V>
inline Matrix
GetSRT(v2t<T>& scale, v2t<U> yp, v2t<V> pos)
{
	Matrix result = Identity();
	SetSRT(result, scale, yp, pos);
	return result;
}

template<typename T, typename U>
inline void
SetSR(Matrix& m, v3t<T> scale, v3t<U> ypr)
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

template<typename T, typename U>
inline void
SetSR(Matrix& m, v2t<T> scale, v2t<U> yp)
{
	SetRotation(m, yp);

	m.xx *= scale.x;
	m.xy *= scale.x;
	m.xz *= scale.x;

	m.yx *= scale.y;
	m.yy *= scale.y;
	m.yz *= scale.y;
}

template<typename T, typename U>
inline Matrix
GetSR(v3t<T>& scale, v3t<U> ypr)
{
	Matrix result = Identity();
	SetSR(result, scale, ypr, pos);
	return result;
}

template<typename T, typename U>
inline Matrix
GetSR(v2t<T> scale, v2t<U> yp)
{
	Matrix result = Identity();
	SetSR(result, scale, yp, pos);
	return result;
}

template<typename T, typename U>
inline void
SetST(Matrix& m, v3t<T> scale, v3t<U> pos)
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

template<typename T, typename U>
inline void
SetST(Matrix& m, v2t<T> scale, v2t<U> pos)
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

template<typename T, typename U>
inline Matrix
GetST(v3t<T>& scale, v3t<U> pos)
{
	Matrix result = Identity();
	SetST(result, scale, pos);
	return result;
}

template<typename T, typename U>
inline Matrix
GetST(v2t<T>& scale, v2t<U> pos)
{
	Matrix result = Identity();
	SetST(result, scale, pos);
	return result;
}

template<typename T, typename U>
inline void
SetRT(Matrix& m, v3t<T> ypr, v3t<U> pos)
{
	SetRotation(m, ypr);

	m.tx = pos.x;
	m.ty = pos.y;
	m.tz = pos.z;
}

template<typename T, typename U>
inline void
SetRT(Matrix& m, v2t<T> yp, v2t<U> pos)
{
	SetRotation(m, yp);

	m.tx = pos.x;
	m.ty = pos.y;
}

template<typename T, typename U>
inline Matrix
GetRT(v3t<T>& ypr, v3t<U> pos)
{
	Matrix result = Identity();
	SetRT(result, ypr, pos);
	return result;
}

template<typename T, typename U>
inline Matrix
GetRT(v2t<T>& yp, v2t<U> pos)
{
	Matrix result = Identity();
	SetRT(result, yp, pos);
	return result;
}

#endif
