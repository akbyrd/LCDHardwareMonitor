#ifndef LHM_RESULT
#define LHM_RESULT

template<typename T>
struct Result
{
	b8 success;
	T  value;

	Result(b8 success) : success(success) { Assert(!success); }
	Result(T value)   : success(true), value(value) {}

	explicit operator b8() { return success; }
	T& operator*() { Assert(success); return value; }
	T& operator->() { Assert(success); return value; }
};

#endif
