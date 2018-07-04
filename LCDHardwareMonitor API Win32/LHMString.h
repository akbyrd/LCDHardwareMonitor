#ifndef LHM_STRING
#define LHM_STRING

// TODO: Seems questionable to alias *_Free(), but nothing else.
// TODO: Static string when capacity < 0?
// TODO: String views or slices (they need to know they don't own the string.
// Actually, the 'static string' concept will work here I think).

// NOTE: Strings are null terminated for C compatibility.

using Bytes   = List<u8>;
using String  = List<c8>;
using WString = List<c16>;

inline void
Bytes_Free(Bytes& bytes)
{
	List_Free(bytes);
}

inline void
String_Free(String& string)
{
	List_Free(string);
}

inline void
WString_Free(WString& string)
{
	List_Free(string);
}

#if false
struct Bytes
{
	u32   length;
	u32   capacity;
	void* data;

	inline operator void*() { return data; }
	inline operator b32()   { return data != nullptr; }
};

struct String
{
	u32 length;
	u32 capacity;
	c8* data;

	inline c8& operator[](i32 i) { return data[i]; }
	inline     operator c8*()    { return data; }
	inline     operator b32()    { return data != nullptr; }
};

struct WString
{
	u32  length;
	u32  capacity;
	c16* data;

	inline c16& operator[](i32 i) { return data[i]; }
	inline      operator c16*()   { return data; }
	inline      operator b32()    { return data != nullptr; }
};

void
FreeBytes(Bytes& bytes)
{
	if (bytes.capacity > 0)
		free(bytes.data);

	bytes = {};
}

void
FreeString(String& string)
{
	if (string.capacity > 0)
		free(string.data);

	string = {};
}

void
FreeWString(WString& string)
{
	if (string.capacity > 0)
		free(string.data);

	string = {};
}
#endif

#endif
