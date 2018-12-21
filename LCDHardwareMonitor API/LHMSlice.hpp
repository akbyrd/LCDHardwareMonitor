#ifndef LHM_SLICE
#define LHM_SLICE

template<typename T>
struct Slice
{
	u32 length;
	u32 stride;
	u8* data;

	// TODO: I wish we could use actual operators for these conversions, rather
	// than constructors.
	Slice()                      { length = 0;           stride = sizeof(T); data = (u8*) nullptr; }
	Slice(u32 _length, T* _data) { length = _length;     stride = sizeof(T); data = (u8*) _data; }
	Slice(const T& element)      { length = 1;           stride = sizeof(T); data = (u8*) &element; }
	Slice(List<T>& list)         { length = list.length; stride = sizeof(T); data = (u8*) list.data; }
	template<u32 Length>
	Slice(const T(&arr)[Length]) { length = Length;      stride = sizeof(T); data = (u8*) arr; }

	using RefT = ListRef<T>;
	inline T& operator[] (RefT r) { return *((T*) &data[stride*(r.index - 1)]); }
	inline T& operator[] (u32 i)  { return *((T*) &data[stride*i]); }
};

template<typename T>
inline b32
List_AppendRange(List<T>& list, Slice<T> items)
{
	if (!List_Reserve(list, list.length + items.length))
		return false;

	if (items.stride == sizeof(T))
	{
		memcpy(&list.data[list.length], items.data, items.length * sizeof(T));
		list.length += items.length;
	}
	else
	{
		for (u32 i = 0; i < items.length; i++)
			list.data[list.length++] = items[i];
	}

	return true;
}

template<typename T>
inline b32
List_Duplicate(Slice<T>& slice, List<T>& duplicate)
{
	List_Reserve(duplicate, slice.length);
	if (!duplicate.data)
		return false;

	duplicate.length   = slice.length;
	duplicate.capacity = slice.length;

	for (u32 i = 0; i < slice.length; i++)
		duplicate[i] = slice[i];

	return true;
}

#endif
