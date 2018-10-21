#ifndef LHM_STRING
#define LHM_STRING


// TODO: I think we want separate implementations for List, Bytes, and Strings
// TODO: Static string when capacity < 0?
// TODO: String views or slices (they need to know they don't own the string.
// Actually, the 'static string' concept will work here I think).

// NOTE: Strings are null terminated for C compatibility.
//
#include <stdarg.h>

using Bytes  = List<u8>;
using String = List<c8>;

b32
String_Format(String& string, c8* format, ...)
{
	Assert(string.data == nullptr);

	String result = {};
	defer { List_Free(result); };

	va_list(args);
	va_start(args, format);
	i32 size = vsnprintf(nullptr, 0, format, args);
	if (size < 0) return false;
	va_end(args);

	b32 success = List_Reserve(result, (u32) size + 1);
	if (!success) return false;
	defer { List_Free(result); };

	va_list(args2);
	va_start(args2, format);
	i32 written = vsnprintf(result.data, result.capacity, format, args2);
	if (written < 0 || (u32) written >= result.capacity) return false;
	result.length = (u32) written + 1;
	va_end(args2);

	string = result;
	result = {};
	return true;
}

#endif
