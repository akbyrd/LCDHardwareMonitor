const float PI = 3.141592654f;

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