#ifndef LHM_STRING
#define LHM_STRING

// TODO: I think we want separate implementations for List, Bytes, and Strings
// TODO: Static string when capacity < 0?
// Actually, the 'static string' concept will work here I think).
// TODO: Can we return the string instead of a bool? It has a bool operator anyway

// NOTE: Strings are null terminated for C compatibility.

using String = List<c8>;

template<typename... Args>
inline b32
String_Format(String& string, c8* format, Args... args)
{
	Assert(string.data == nullptr);

	String result = {};
	defer { List_Free(result); };

	i32 size = snprintf(nullptr, 0, format, args...);
	if (size < 0) return false;

	b32 success = List_Reserve(result, (u32) size + 1);
	if (!success) return false;
	defer { List_Free(result); };

	i32 written = snprintf(result.data, result.capacity, format, args...);
	if (written < 0 || (u32) written >= result.capacity) return false;
	result.length = (u32) written + 1;

	string = result;
	result = {};
	return true;
}

#endif
