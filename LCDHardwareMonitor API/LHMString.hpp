#ifndef LHM_STRING
#define LHM_STRING

// NOTE: Strings and StringViews are null terminated for C compatibility.
// NOTE: StringSlices are NOT GUARANTEED to be null terminated.
// NOTE: The capacity of a String DOES include the null terminator
// NOTE: The length of a String DOES NOT include the null terminator
// NOTE: The length of a StringView DOES NOT include the null terminator
// NOTE: The length of a StringSlice DOES NOT include the null terminator (if present)

// TODO: Maybe capacity should not include the null terminator
// TODO: How do we handle immutable strings? Since there's no const transitivity, do we need separate types?

struct StrPos
{
	u32 value;

	static const StrPos Null;

	explicit operator b8() { return *this != Null; }
	b8     operator== (StrPos rhs) { return value == rhs.value; }
	b8     operator!= (StrPos rhs) { return value != rhs.value; }
	StrPos operator+  (i32 rhs)    { return { value + rhs }; }
	StrPos operator-  (i32 rhs)    { return { value - rhs }; }
};

inline u32 ToIndex(StrPos pos) { return pos.value - 1; }
inline u32 ToStrPos(u32 index) { return { index + 1 }; }

const StrPos StrPos::Null = {};

struct String
{
	u32 length;
	u32 capacity;
	c8* data;

	c8& operator[] (StrPos p) { u32 i = ToIndex(p); Assert(i < length); return data[i]; }
	c8& operator[] (u32 i)    { Assert(i < length); return data[i]; }
};

void String_Free(String& string);
using ScopedString = Scoped<String, String_Free>;

struct StringView
{
	u32 length;
	c8* data;

	StringView()                       { length = 0;             data = nullptr;     }
	StringView(const String& string)   { length = string.length; data = string.data; }
	template<u32 Length>
	StringView(const c8(&arr)[Length]) { length = Length - 1;    data = (c8*) arr;   }

	c8& operator[] (StrPos p) { u32 i = ToIndex(p); Assert(i < length); return data[i]; }
	c8& operator[] (u32 i)    { Assert(i < length); return data[i]; }
};

struct StringSlice
{
	u32 length;
	c8* data;

	StringSlice()                         { length = 0;             data = nullptr;     }
	StringSlice(const String& string)     { length = string.length; data = string.data; }
	StringSlice(const StringView& string) { length = string.length; data = string.data; }
	template<u32 Length>
	StringSlice(const c8(&arr)[Length])   { length = Length - 1;    data = (c8*) arr;   }

	c8& operator[] (StrPos p) { u32 i = ToIndex(p); Assert(i < length); return data[i]; }
	c8& operator[] (u32 i)    { Assert(i < length); return data[i]; }
};

// -------------------------------------------------------------------------------------------------
// String API

inline void
String_Free(String& string)
{
	Free(string.data);
	string = {};
}

inline void
String_Reserve(String& string, u32 capacity)
{
	if (string.capacity < capacity)
	{
		u64 totalSize = capacity;
		u64 emptySize = (capacity - string.length - 1);

		string.capacity = capacity;
		string.data     = (c8*) ReallocChecked(string.data, (size) totalSize);
		memset(&string.data[string.length], 0, (size) emptySize);
	}
}

// TODO: See what the errors are like when format isn't a c8[] or isn't constexpr
#define String_Format(format, ...) \
	String_FormatChecked<CountPlaceholders(format)>(format, ##__VA_ARGS__)

String
String_FromView(StringView view)
{
	String string = {};
	String_Reserve(string, view.length + 1);

	string.length = view.length;
	strncpy_s(string.data, string.capacity, view.data, view.length);

	return string;
}

String
String_FromSlice(StringSlice slice)
{
	String string = {};
	String_Reserve(string, slice.length + 1);

	string.length = slice.length;
	strncpy_s(string.data, string.capacity, slice.data, slice.length);

	return string;
}

inline StrPos
String_GetPos(StringView string, u32 index)
{
	UNUSED(string);
	Assert(index < string.length);
	StrPos pos = {};
	pos.value = ToStrPos(index);
	return pos;
}

inline StrPos
String_GetFirstPos(StringView string)
{
	return String_GetPos(string, 0);
}

inline StrPos
String_GetLastPos(StringView string)
{
	return String_GetPos(string, string.length - 1);
}

inline StrPos
String_FindFirst(StringView string, c8 item)
{
	for (u32 i = 0; i < string.length; i++)
		if (string.data[i] == item)
			return String_GetPos(string, i);

	return StrPos::Null;
}

inline StrPos
String_FindLast(StringView string, c8 item)
{
	for (u32 i = string.length; i > 0; i--)
		if (string.data[i - 1] == item)
			return String_GetPos(string, i - 1);

	return StrPos::Null;
}

// -------------------------------------------------------------------------------------------------
// StringSlice API

StringSlice
StringSlice_Create(StringView string, StrPos first, StrPos last)
{
	StringSlice slice = {};
	slice.data    = &string[first];
	slice.length = last.value - first.value + 1;
	return slice;
}

// -------------------------------------------------------------------------------------------------
// String Formatting Implementation

constexpr u32
CountPlaceholders(const c8* format)
{
	u32 count = 0;
	for (const c8* c = format; *c; c++)
	{
		c8 ahead0 = c[0];
		c8 ahead1 = c[0] ? c[1] : '\0';
		c8 ahead2 = c[0] && c[1] ? c[2] : '\0';
		count += (ahead0 == '%' && (ahead1 != '!' || ahead2 == '!'));
	}
	return count;
}

template<typename Arg0, typename... Args>
inline u32
FormatImplRecurse(String& string, StringView format, u32 iFmt, Arg0&& arg0, Args&&... args)
{
	u32 written = ToString(string, arg0);
	return written + FormatImpl(string, format, iFmt + 1, args...);
}

// TODO: Rename this
template<typename... Args>
inline u32
FormatImpl(String& string, StringView format, u32 iFmt, Args&&... args)
{
	b8 lengthOnly = !string.data;
	u32 written = 0;

	auto Step = [&](u32 strCount, u32 fmtCount) {
		written += strCount;
		string.length += strCount * !lengthOnly;
		iFmt += fmtCount;
	};

	while (iFmt < format.length)
	{
		if (format[iFmt] != '%')
		{
			u32 len;
			for (len = 1; iFmt + len < format.length; len++)
				if (format[iFmt + len] == '%') break;

			if (!lengthOnly) strncpy_s(&string.data[string.length], string.capacity - string.length, &format[iFmt], len);
			Step(len, len);
		}
		else
		{
			c8 ahead1 = format.length - iFmt > 1 ? format[iFmt + 1] : '\0';
			c8 ahead2 = format.length - iFmt > 2 ? format[iFmt + 2] : '\0';

			if (ahead1 != '!' || ahead2 == '!')
			{
				if constexpr (sizeof...(args) > 0)
					return written + FormatImplRecurse(string, format, iFmt, args...);
				Assert(false);
			}
			else if (ahead1 == '!' && ahead2 != '!')
			{
				if (!lengthOnly)
				{
					string.data[string.length + 0] = '%';
					string.data[string.length + 1] = '\0';
				}
				Step(1, 2);
			}
			else if (ahead1 == '!' && ahead2 == '!')
			{
				if (!lengthOnly)
				{
					string.data[string.length + 0] = '!';
					string.data[string.length + 1] = '\0';
				}
				Step(1, 2);
			}
		}
	}

	Assert(lengthOnly || string.data[string.length] == '\0');
	return written;
}

template<typename... Args>
String
String_FormatImpl(StringView format, Args&&... args)
{
	String string = {};

	u32 stringLen = FormatImpl(string, format, 0, args...);
	String_Reserve(string, stringLen + 1);

	FormatImpl(string, format, 0, args...);
	Assert(string.length == stringLen);

	return string;
}

template<u32 PlaceholderCount, typename... Args>
inline String
String_FormatChecked(StringView format, Args&&... args)
{
	static_assert(PlaceholderCount == sizeof...(args));
	return String_FormatImpl(format, args...);
}

// -------------------------------------------------------------------------------------------------
// Primitive ToString Implementations

template<typename T>
inline u32
ToStringImpl_Format(String& string, StringView format, T value)
{
	b8 lengthOnly = !string.data;
	u32 written = 0;
	// TODO: Check return value!
	written += (u32) snprintf(&string.data[string.length], string.capacity - string.length, format.data, value);
	string.length += written * !lengthOnly;
	return written;
}

// TODO: Might want to add a 'context' parameter that can keep track of nested indentation
u32 ToString(String& string, u8        value) { return ToStringImpl_Format(string, "%hu",  value); }
u32 ToString(String& string, u16       value) { return ToStringImpl_Format(string, "%u",   value); }
u32 ToString(String& string, u32       value) { return ToStringImpl_Format(string, "%lu",  value); }
u32 ToString(String& string, u64       value) { return ToStringImpl_Format(string, "%llu", value); }
u32 ToString(String& string, i8        value) { return ToStringImpl_Format(string, "%hi",  value); }
u32 ToString(String& string, i16       value) { return ToStringImpl_Format(string, "%i",   value); }
u32 ToString(String& string, i32       value) { return ToStringImpl_Format(string, "%li",  value); }
u32 ToString(String& string, i64       value) { return ToStringImpl_Format(string, "%lli", value); }
u32 ToString(String& string, r32       value) { return ToStringImpl_Format(string, "%f",   value); }
u32 ToString(String& string, r64       value) { return ToStringImpl_Format(string, "%f",   value); }
u32 ToString(String& string, c8        value) { return ToStringImpl_Format(string, "%c",   value); }
u32 ToString(String& string, const c8* value) { return ToStringImpl_Format(string, "%s",   value); }
u32 ToString(String& string, b8        value) { return ToStringImpl_Format(string, "%s",   value ? "true" : "false"); }

u32
ToString(String& string, String value)
{
	b8 lengthOnly = !string.data;
	u32 written = value.length;
	if (!lengthOnly) strncpy_s(&string.data[string.length], string.capacity - string.length, value.data, value.length);
	string.length += written * !lengthOnly;
	return written;
}

u32
ToString(String& string, StringView value)
{
	b8 lengthOnly = !string.data;
	u32 written = value.length;
	if (!lengthOnly) strncpy_s(&string.data[string.length], string.capacity - string.length, value.data, value.length);
	string.length += written * !lengthOnly;
	return written;
}

u32
ToString(String& string, StringSlice value)
{
	b8 lengthOnly = !string.data;
	u32 written = value.length;
	if (!lengthOnly) strncpy_s(&string.data[string.length], string.capacity - string.length, value.data, value.length);
	string.length += written * !lengthOnly;
	return written;
}

template<typename T>
u32
ToString(String& string, List<T>& list)
{
	u32 written = 0;
	if (list.length == 0)
	{
		written += ToString(string, "List: {}");
	}
	else
	{
		written += ToString(string, "List: {\n");
		for (u32 i = 0; i < list.length; i++)
		{
			written += ToString(string, "\t");
			written += ToString(string, list[i]);
			written += ToString(string, ",\n");
		}
		written += ToString(string, "}");
	}
	return written;
}

template<typename T>
u32
ToString(String& string, Slice<T>& list)
{
	u32 written = 0;
	if (list.length == 0)
	{
		written += ToString(string, "Slice: {}");
	}
	else
	{
		written += ToString(string, "Slice: {\n");
		for (u32 i = 0; i < list.length; i++)
		{
			written += ToString(string, "\t");
			written += ToString(string, list[i]);
			written += ToString(string, ",\n");
		}
		written += ToString(string, "}");
	}
	return written;
}

#endif
