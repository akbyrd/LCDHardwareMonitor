#ifndef LHM_SLICE
#define LHM_SLICE

// TODO: Move the conversion operator in List<T> to Slice<T>
template<typename T>
struct Slice
{
	u32 length;
	T*  data;

	// TODO: I wish we could use actual operators for these conversions, rather
	// then constructors.
	Slice() { length = 0; data = nullptr; }
	Slice(u32 length, T* data) { Slice::length = length; Slice::data = data; }
	Slice(T& element) { length = 1; data = &element; }
	Slice(List<T>& list) { length = list.length; data = list.data; }

	template<u32 Length>
	Slice(T(&arr)[Length]) { length = Length; data = arr; }

	inline T& operator [](u32 i) { return data[i]; }

	// TODO: There's no guarantee a ref is even valid on a slice. I'm not sure
	// this should exist.
	using RefT = ListRef<T>;
	inline T& operator [](RefT r) { return data[r.index]; }
};

//template<typename T>
//struct StridedSlice
//{
//	u32 length;
//	u32 stride;
//	T*  data;
//
//	inline T& operator [](u32 i) { return *(T*) (((u8*) data) + (i*stride)); }
//};

template<typename T>
inline b32
List_AppendRange(List<T>& list, Slice<T> items)
{
	if (!List_Reserve(list, list.length + items.length))
		return false;

	memcpy(&list.data[list.length], items.data, items.length);

	return true;
}

#endif
