#ifndef LHM_HASH
#define LHM_HASH

// NOTE: This is somewhat weak for short, similar strings. Consider FNV or Murmur.
constexpr u32 Adler32(const c8* data, size length)
{
	if (length == 0) return 0;

	u32 a = 1;
	u32 b = 0;
	for (size i = 0; i < length; i++)
	{
		c8 d = data[i];
		a = (a + d) % 65521;
		b = (b + a) % 65521;
	}

	return (b << 16) | a;
}

template<size N>
constexpr u32 Adler32(const char (&data)[N])
{
	return Adler32(data, N);
}

template<typename T>
constexpr u32 _IdOf()
{
	return Adler32(__FUNCTION_FULL_NAME__);
}

template<class T>
constexpr u32 IdOf = _IdOf<T>();

#endif
