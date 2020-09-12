template <typename T, typename U>
void
TestV2()
{
	v2t<T> a = {};
	v2t<T> b = {};

	a == b;
	a != b;

	if constexpr (IsSigned<T>)
	{
		+a;
		-a;
	}

	a + b;
	a - b;
	a * b;
	a / b;

	a += b;
	a -= b;
	a *= b;
	a /= b;

	T c = 0;

	a * c;
	c * a;
	a / c;
	c / a;

	a *= c;
	a /= c;

	for (u32 i = 0; i < ArrayLength(a.arr); i++)
	{
		a[i];
		a.arr[i];

		const v2t<T> d = {};
		d[i];
		d.arr[i];
	}

	(v2t<U>) a;
	(v3t<T>) a;
	(v4t<T>) a;

	Clamp(a, b, b);
	Dot(a, b);
	Lerp(a, b, 0.0f);
	Max(a, b);
	Min(a, b);

	if constexpr (IsSameType<T, r32> || IsSameType<T, r64>)
	{
		Normalize(a);
	}
}

template <typename T, typename U>
void
TestV3()
{
	v3t<T> a = {};
	v3t<T> b = {};

	a == b;
	a != b;

	if constexpr (IsSigned<T>)
	{
		+a;
		-a;
	}

	a + b;
	a - b;
	a * b;
	a / b;

	a += b;
	a -= b;
	a *= b;
	a /= b;

	T c = 0;

	a * c;
	c * a;
	a / c;
	c / a;

	a *= c;
	a /= c;

	for (u32 i = 0; i < ArrayLength(a.arr); i++)
	{
		a[i];
		a.arr[i];

		const v3t<T> d = {};
		d[i];
		d.arr[i];
	}

	(v2t<T>) a;
	(v3t<U>) a;
	(v4t<T>) a;

	Clamp(a, b, b);
	Cross(a, b);
	Dot(a, b);
	Lerp(a, b, 0.0f);
	Max(a, b);
	Min(a, b);

	if constexpr (IsSameType<T, r32> || IsSameType<T, r64>)
	{
		GetOrbitPos(a, v2t<T> {}, (T) 0.0);
		Normalize(a);
	}
}

template <typename T, typename U>
void
TestV4()
{
	v4t<T> a = {};
	v4t<T> b = {};

	a == b;
	a != b;

	if constexpr (IsSigned<T>)
	{
		+a;
		-a;
	}

	a + b;
	a - b;
	a * b;
	a / b;

	a += b;
	a -= b;
	a *= b;
	a /= b;

	T c = 0;

	a * c;
	c * a;
	a / c;
	c / a;

	a *= c;
	a /= c;

	for (u32 i = 0; i < ArrayLength(a.arr); i++)
	{
		a[i];
		a.arr[i];

		const v4t<T> d = {};
		d[i];
		d.arr[i];
	}

	(v2t<T>) a;
	(v3t<T>) a;
	(v4t<U>) a;

	Clamp(a, b, b);
	Dot(a, b);
	Lerp(a, b, 0.0f);
	Max(a, b);
	Min(a, b);

	if constexpr (IsSameType<T, r32> || IsSameType<T, r64>)
	{
		Normalize(a);
	}
}

void
TestMatrix()
{
	Matrix m = {};

	v4t<r32> v = {};
	v * m;
	v *= m;
}

template <typename T>
void
TestRect()
{
	v4t<T> a = {};
	v2t<T> b = {};

	ClampRect(a, v4t<T> {});
	GrowRect(a, (T) 0);
	GrowRect(a, v2t<T> {});
	GrowRect(a, v4t<T> {});
	MakeRect(b, (T) 0);
	MakeRect(b, v2t<T> {});
	MakeRect(b, v4t<T> {});
	RectContains(a, v2t<T> {});
	RectCombine(a, v4t<T> {});
}

template <typename T>
void TestScalar()
{
	T a = {};
	T b = {};
	T c = {};
	r32 t = {};

	if constexpr (IsSigned<T>)
	{
		Abs(a);
	}

	if constexpr (!IsFloat<T>)
	{
		IsMultipleOf(a, b);
	}

	Clamp(a, b, c);
	Lerp(a, b, t);
	Max(a, b);
	Min(a, b);
}

void
TestMath()
{
	TestV2<r32, i32>();
	TestV2<r64, i32>();
	TestV2<i8,  i32>();
	TestV2<i16, i32>();
	TestV2<i32, u32>();
	TestV2<i64, i32>();
	TestV2<u8,  i32>();
	TestV2<u16, i32>();
	TestV2<u32, i32>();
	TestV2<u64, i32>();

	TestV3<r32, i32>();
	TestV3<r64, i32>();
	TestV3<i8,  i32>();
	TestV3<i16, i32>();
	TestV3<i32, u32>();
	TestV3<i64, i32>();
	TestV3<u8,  i32>();
	TestV3<u16, i32>();
	TestV3<u32, i32>();
	TestV3<u64, i32>();

	TestV4<r32, i32>();
	TestV4<r64, i32>();
	TestV4<i8,  i32>();
	TestV4<i16, i32>();
	TestV4<i32, u32>();
	TestV4<i64, i32>();
	TestV4<u8,  i32>();
	TestV4<u16, i32>();
	TestV4<u32, i32>();
	TestV4<u64, i32>();

	TestMatrix();

	TestRect<r32>();
	TestRect<r64>();
	TestRect<i8>();
	TestRect<i16>();
	TestRect<i32>();
	TestRect<i64>();
	TestRect<u8>();
	TestRect<u16>();
	TestRect<u32>();
	TestRect<u64>();

	TestScalar<r32>();
	TestScalar<r64>();
	TestScalar<i8>();
	TestScalar<i16>();
	TestScalar<i32>();
	TestScalar<i64>();
	TestScalar<u8>();
	TestScalar<u16>();
	TestScalar<u32>();
	TestScalar<u64>();
}

// TODO: Unary +
// TODO: Remove second template parameters
// TODO: Add v<T> *=, /, /=
// TODO: Change to v<T> for +=, -=,
// TODO: Limit Normalize to floats and non-constexpr
// TODO: Make constexpr
// TODO: Add casting
// TODO: Extra functions


// TODO: Rename multiplier, divisor, dividend, v
// TODO: Unify contruction / initialization style
