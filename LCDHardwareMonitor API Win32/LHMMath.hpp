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

union v2i
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
operator== (v2i lhs, v2i rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

inline b32
operator!= (v2i lhs, v2i rhs)
{
	return !(lhs.x == rhs.x && lhs.y == rhs.y);
}

inline v2i
operator+ (v2i lhs, v2i rhs)
{
	return {lhs.x + rhs.x, lhs.y + rhs.y};
}

inline v2i
operator- (v2i lhs, v2i rhs)
{
	return {lhs.x - rhs.x, lhs.y - rhs.y};
}

inline v2i
operator* (i32 multiplier, v2i v)
{
	return {multiplier * v.x, multiplier * v.y};
}

inline v2i
operator/ (v2i v, i32 dividend)
{
	return {v.x / dividend, v.y / dividend};
}

inline i32
v2i::operator[] (i32 index)
{
	return arr[index];
}

inline void
Clamp(v2i v, v2i maxSize)
{
	if (v.x > maxSize.x) v.x = maxSize.x;
	if (v.y > maxSize.y) v.y = maxSize.y;
}


union v3
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
operator== (v3 lhs, v3 rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

inline b32
operator!= (v3 lhs, v3 rhs)
{
	return !(lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z);
}

inline v3
operator+ (v3 lhs, v3 rhs)
{
	return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

inline v3
operator- (v3 lhs, v3 rhs)
{
	return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

inline v3
operator* (r32 multiplier, v3 v)
{
	return {multiplier * v.x, multiplier * v.y, multiplier * v.z};
}

inline v3
operator/ (v3 v, r32 dividend)
{
	return {v.x / dividend, v.y / dividend, v.z / dividend};
}

inline r32
v3::operator[] (i32 index)
{
	return arr[index];
}

inline void
Clamp(v3 v, v3 maxSize)
{
	if (v.x > maxSize.x) v.x = maxSize.x;
	if (v.y > maxSize.y) v.y = maxSize.y;
	if (v.z > maxSize.z) v.y = maxSize.z;
}


union v4
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
operator== (v4 lhs, v4 rhs)
{
	return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

inline b32
operator!= (v4 lhs, v4 rhs)
{
	return !(lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w);
}

inline v4
operator+ (v4 lhs, v4 rhs)
{
	return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

inline v4
operator- (v4 lhs, v4 rhs)
{
	return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

inline v4
operator* (r32 multiplier, v4 v)
{
	return {multiplier * v.x, multiplier * v.y, multiplier * v.z, multiplier * v.w};
}

inline v4
operator/ (v4 v, r32 dividend)
{
	return {v.x / dividend, v.y / dividend, v.z / dividend, v.w / dividend};
}

inline r32
v4::operator[] (i32 index)
{
	return arr[index];
}


inline void
Clamp(v4 v, v4 maxSize)
{
	if (v.x > maxSize.x) v.x = maxSize.x;
	if (v.y > maxSize.y) v.y = maxSize.y;
	if (v.z > maxSize.z) v.z = maxSize.z;
	if (v.w > maxSize.w) v.w = maxSize.w;
}

#endif
