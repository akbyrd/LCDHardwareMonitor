#ifndef LHM_MATH
#define LHM_MATH

const i32 i32Min = -2147483647 - 1;
const i32 i32Max = 2147483647;
const u32 u32Max = 4294967295;
const r32 r32Min = 1.175494351e-38f;
const r32 r32Max = 3.402823466e+38f;

const r32 r32Epsilon = 31.192092896e-07f;
const r32 r32Pi      = 3.141592654f;


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

union V2i
{
	struct
	{
		i32 x;
		i32 y;
	};

	// Aliases
	i32 arr[2];

	i32 operator[] (i32 index);
};

// Operators
inline b32
operator== (V2i lhs, V2i rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline b32
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
operator* (i32 multiplier, V2i v)
{
	return {multiplier * v.x, multiplier * v.y};
}

inline V2i
operator/ (V2i v, i32 dividend)
{
	return {v.x / dividend, v.y / dividend};
}

inline i32
V2i::operator[] (i32 index)
{
	return arr[index];
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

	// Aliases
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

	r32 operator[] (i32 index);
};

// Operators
inline b32
operator== (V3 lhs, V3 rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

inline b32
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
operator* (r32 multiplier, V3 v)
{
	return {multiplier * v.x, multiplier * v.y, multiplier * v.z};
}

inline V3
operator/ (V3 v, r32 dividend)
{
	return {v.x / dividend, v.y / dividend, v.z / dividend};
}

inline r32
V3::operator[] (i32 index)
{
	return arr[index];
}

inline void
Clamp(V3 v, V3 maxSize)
{
	if (v.x > maxSize.x) v.x = maxSize.x;
	if (v.y > maxSize.y) v.y = maxSize.y;
	if (v.z > maxSize.z) v.y = maxSize.z;
}


union V4
{
	struct
	{
		r32 x;
		r32 y;
		r32 z;
		r32 w;
	};

	// Aliases
	struct
	{
		r32 r;
		r32 g;
		r32 b;
		r32 a;
	};
	r32 arr[4];

	r32 operator[] (i32 index);
};

// Operators
inline b32
operator== (V4 lhs, V4 rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

inline b32
operator!= (V4 lhs, V4 rhs)
{
	return !(lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w);
}

inline V4
operator+ (V4 lhs, V4 rhs)
{
	return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

inline V4
operator- (V4 lhs, V4 rhs)
{
	return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

inline V4
operator* (r32 multiplier, V4 v)
{
	return {multiplier * v.x, multiplier * v.y, multiplier * v.z, multiplier * v.w};
}

inline V4
operator/ (V4 v, r32 dividend)
{
	return {v.x / dividend, v.y / dividend, v.z / dividend, v.w / dividend};
}

inline r32
V4::operator[] (i32 index)
{
	return arr[index];
}


inline void
Clamp(V4 v, V4 maxSize)
{
	if (v.x > maxSize.x) v.x = maxSize.x;
	if (v.y > maxSize.y) v.y = maxSize.y;
	if (v.z > maxSize.z) v.z = maxSize.z;
	if (v.w > maxSize.w) v.w = maxSize.w;
}

#endif
