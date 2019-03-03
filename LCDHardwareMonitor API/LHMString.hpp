#ifndef LHM_STRING
#define LHM_STRING

// NOTE: Strings are null terminated for C compatibility.
// NOTE: StringSlices are NOT null terminated.
// NOTE: The length of a String DOES include the null terminator
// NOTE: The length of a StringSlice DOES include the null terminator
// TODO: Length should not include the null terminator

using String       = List<c8>;
using StringSlice  = Slice<c8>;
using ScopedString = Scoped<String, decltype(List_Free<c8>), List_Free<c8>>;

template<typename... Args>
inline b32
String_Format(String& string, c8* format, Args... args)
{
	bool preallocated = string.data != nullptr;
	auto allocGuard = guard { if (!preallocated) List_Free(string); };

	i32 size = snprintf(nullptr, 0, format, args...);
	if (size < 0) return false;

	b32 success = List_Reserve(string, (u32) size + 1);
	if (!success) return false;

	i32 written = snprintf(string.data, string.capacity, format, args...);
	if (written < 0 || (u32) written >= string.capacity) return false;
	string.length = (u32) written + 1;

	allocGuard.dismiss = true;
	return true;
}

inline b32
String_FromSlice(String& string, StringSlice slice)
{
	bool preallocated = string.data != nullptr;
	auto allocGuard = guard { if (!preallocated) List_Free(string); };

	b32 success = List_Reserve(string, slice.length + 1);
	if (!success) return false;

	string.length = 0;
	List_AppendRange(string, slice);
	string[string.length++] = 0;

	allocGuard.dismiss = true;
	return true;
}

#endif
