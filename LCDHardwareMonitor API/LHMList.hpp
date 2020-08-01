#ifndef LHM_LIST
#define LHM_LIST

// NOTE:
// - Memory is zero-initialized during allocations and when items are removed.
//   If you choose to manipulate internal data such as the current length (e.g.
//   to remove an item) you will leave dangling data whose destructor has not
//   been called. If an item is later placed into that spot the destructor will
//   be called upon the copy-assignment. Using the provided functions is
//   recommended.
//
// - Does *not* support move semantics. Items are copy-assigned into slots.
//   Using classes with single ownership semantics (i.e. ones that will free a
//   resource in the destructor) are not recommended.
//
// - List_Contains is determined by reference, not by value. That is, the
//   address of the object is checked to see if it lies within the list.
//
// - Grow and Append can fail due to allocations.
//   Grow returns false.
//   Append returns a nullptr instead of a pointer to the new slot.

// TODO: Consider renaming this to Handle or something. Ref is overloaded
template<typename T>
struct ListRef
{
	u32 value;

	static const ListRef<T> Null;
	explicit operator b8() { return *this != Null; }
};

template<typename T>
const ListRef<T> ListRef<T>::Null = {};

template<typename T>
inline b8 operator== (ListRef<T> lhs, ListRef<T> rhs) { return lhs.value == rhs.value; }

template<typename T>
inline b8 operator!= (ListRef<T> lhs, ListRef<T> rhs) { return lhs.value != rhs.value; }

template<typename T>
inline u32 ToIndex (ListRef<T> ref) { return ref.value - 1; }

template<typename T>
inline ListRef<T> ToRef (u32 index) { return { index + 1 }; }

template<typename T>
struct List
{
	u32 length;
	u32 capacity;
	T*  data;

	using RefT = ListRef<T>;
	inline T& operator[] (RefT r) { u32 i = ToIndex(r); Assert(i < length); return data[i]; }
	inline T& operator[] (u32 i)  { Assert(i < length); return data[i]; }
};

#if false
template<typename T>
inline void
List_Free(List<T>& list);

template<typename T>
using ScopedList = Scoped<List<T>, List_Free<T>>;
#endif

template<typename T>
struct Slice
{
	u32 length;
	u32 stride;
	T*  data;

	                     Slice()                      { length = 0;           stride = sizeof(T); data = nullptr;        }
	                     Slice(const T& element)      { length = 1;           stride = sizeof(T); data = (T*) &element;  }
	                     Slice(const List<T>& list)   { length = list.length; stride = sizeof(T); data = (T*) list.data; }
	template<u32 Length> Slice(const T(&arr)[Length]) { length = Length;      stride = sizeof(T); data = (T*) arr;       }

	//operator Slice<Slice<T>>() { return { 1, sizeof(Slice<T>), this }; }

	template<typename U>
	explicit operator Slice<U>() { return *(Slice<U>*) this; }

	using RefT = ListRef<T>;
	inline T& operator[] (RefT r) { u32 i = ToIndex(r); Assert(i < length); return (T&) ((u8*) data)[stride*i]; }
	inline T& operator[] (u32 i)  { Assert(i < length); return (T&) ((u8*) data)[stride*i]; }
};

template<typename T>
inline void
List_Reserve(List<T>& list, u32 capacity)
{
	if (list.capacity < capacity)
	{
		u64 totalSize = sizeof(T) * capacity;
		u64 emptySize = sizeof(T) * (capacity - list.length);

		list.capacity = capacity;
		list.data     = (T*) ReallocChecked(list.data, (size) totalSize);
		memset(&list.data[list.length], 0, (size) emptySize);
	}
}

template<typename T>
inline void
List_Grow(List<T>& list)
{
	if (list.length == list.capacity)
	{
		u32 capacity  = list.capacity ? 2*list.capacity : 4;
		u64 totalSize = sizeof(T) * capacity;
		u64 emptySize = sizeof(T) * (capacity - list.length);

		list.capacity = capacity;
		list.data     = (T*) ReallocChecked(list.data, (size) totalSize);
		memset(&list.data[list.length], 0, (size) emptySize);
	}
}

template<typename T>
inline T&
List_Append(List<T>& list, T item)
{
	List_Grow(list);
	T& slot = list[list.length++];
	slot = item;
	return slot;
}

template<typename T>
inline T&
List_Append(List<T>& list)
{
	List_Grow(list);
	T& slot = list[list.length++];
	return slot;
}

template<typename T>
inline void
List_AppendRange(List<T>& list, u32 count)
{
	List_Reserve(list, list.length + count);
	list.length += count;
}

template<typename T>
inline void
List_AppendRange(List<T>& list, Slice<T> items)
{
	List_Reserve(list, list.length + items.length);
	if (items.stride == sizeof(T))
	{
		memcpy(&list.data[list.length], items.data, sizeof(T) * items.length);
		list.length += items.length;
	}
	else
	{
		for (u32 i = 0; i < items.length; i++)
			list[list.length++] = items[i];
	}
}

template<typename T>
inline void
List_Clear(List<T>& list)
{
	list.length = 0;
	memset(list.data, 0, sizeof(T) * list.capacity);
}

template<typename T>
inline b8
List_Contains(List<T>& list, T item)
{
	if (list.length == 0)
		return false;

	for (u32 i = 0; i < list.length; i++)
	{
		if (list[i] == item)
			return true;
	}
	return false;
}

template<typename T>
inline List<T>
List_Duplicate(Slice<T>& slice)
{
	List<T> duplicate = {};
	List_AppendRange(duplicate, slice);
	return duplicate;
}

template<typename T>
inline b8
List_Equal(List<T>& lhs, List<T>& rhs)
{
	if (lhs.length != rhs.length)
		return false;

	for (u32 i = 0; i < lhs.length; i++)
	{
		if (lhs[i] != rhs[i])
			return false;
	}

	return true;
}

template<typename T>
inline ListRef<T>
List_FindFirst(List<T>& list, T item)
{
	for (u32 i = 0; i < list.length; i++)
		if (list[i] == item)
			return List_GetRef(list, i);

	return ListRef<T>::Null;
}

template<typename T>
inline ListRef<T>
List_FindLast(List<T>& list, T item)
{
	for (u32 i = list.length; i > 0; i--)
		if (list[i - 1] == item)
			return List_GetRef(list, i);

	return ListRef<T>::Null;
}

template<typename T>
inline void
List_Free(List<T>& list)
{
	Free(list.data);
	list = {};
}

template<typename T>
inline T&
List_Get(List<T>& list, u32 index)
{
	return list[index];
}

template<typename T>
inline T&
List_GetFirst(List<T>& list)
{
	return list[0];
}

template<typename T>
inline T&
List_GetLast(List<T>& list)
{
	Assert(list.length > 0);
	return list[list.length - 1];
}

template<typename T>
inline ListRef<T>
List_GetRef(List<T>& list, u32 index)
{
	UNUSED(list);
	Assert(index < list.length);
	return ToRef<T>(index);
}

template<typename T>
inline ListRef<T>
List_GetFirstRef(List<T>& list)
{
	return List_GetRef(list, 0);
}

template<typename T>
inline ListRef<T>
List_GetLastRef(List<T>& list)
{
	return List_GetRef(list, list.length - 1);
}

template<typename T>
inline u32
List_GetRemaining(List<T>& list)
{
	return list.capacity - list.length;
}

template<typename T>
inline b8
List_IsRefValid(List<T>& list, ListRef<T> ref)
{
	b8 result = ToIndex(ref) < list.length;
	return result;
}

template<typename T>
inline T&
List_Push(List<T>& list)
{
	return List_Append(list);
}

template<typename T>
inline T
List_Pop(List<T>& list)
{
	T item = List_GetLast(list);
	List_RemoveLast(list);
	return item;
}

template<typename T>
inline void
List_Remove(List<T>& list, ListRef<T> ref)
{
	Assert(List_IsRefValid(list, ref));
	u32 index = ToIndex(ref);
	memmove(&list[index], &list[index + 1], sizeof(T) * (list.length - index - 1));
	List_ZeroRange(list, list.length - 1, 1);
	list.length--;
}

template<typename T>
inline void
List_Remove(List<T>& list, u32 index)
{
	Assert(index < list.length);
	memmove(&list[index], &list.data[index + 1], sizeof(T) * (list.length - index - 1));
	List_ZeroRange(list, list.length - 1, 1);
	list.length--;
}

template<typename T>
inline void
List_RemoveFast(List<T>& list, ListRef<T> ref)
{
	Assert(List_IsRefValid(list, ref));
	T& slot = list[ref];
	slot = list[list.length - 1];
	List_ZeroRange(list, list.length - 1, 1);
	list.length--;
}

template<typename T>
inline void
List_RemoveFast(List<T>& list, u32 index)
{
	Assert(index < list.length);
	T& slot = list[index];
	slot = list[list.length - 1];
	List_ZeroRange(list, list.length - 1, 1);
	list.length--;
}

// TODO: Consider adding some special data structure for lists of untyped bytes
template<typename T>
inline void
List_RemoveRangeFast(List<T>& list, u32 start, u32 count)
{
	Assert(start < list.length && (start + count) <= list.length);
	memcpy(&list[start], &list[start + count], sizeof(T) * count);
	List_ZeroRange(list, list.length - count, 1);
	list.length -= count;
}

template<typename T>
inline void
List_RemoveLast(List<T>& list)
{
	Assert(list.length > 0);
	memset(&list[list.length - 1], 0, sizeof(T));
	list.length--;
}

template<typename T>
inline void
List_Shrink(List<T>& list)
{
	u32 capacity = list.capacity / 2;
	if (list.length < capacity)
	{
		u64 totalSize = sizeof(T) * capacity;
		list.capacity = capacity;
		list.data     = (T*) ReallocChecked(list.data, (size) totalSize);
	}

	return true;
}

// TODO: Consider renaming to List_SizeOfData
template<typename T>
inline u32
List_SizeOf(List<T>& list)
{
	u32 result = sizeof(T) * list.length;
	return result;
}

template<typename T>
inline u32
List_SizeOf(Slice<T>& slice)
{
	u32 result = sizeof(T) * slice.length;
	return result;
}

template<typename T>
inline u32
List_SizeOfRemaining(List<T>& list)
{
	u32 result = sizeof(T) * (list.capacity - list.length);
	return result;
}

template<typename T>
inline void
List_ZeroRange(List<T>& list, u32 start, u32 count)
{
	Assert(start < list.length && (start + count) <= list.length);
	memset(&list[start], 0, sizeof(T) * count);
}

template<typename T>
inline Slice<T>
List_Slice(List<T>& list, u32 start)
{
	Slice<T> result = {};
	result.length = list.length - start;
	result.stride = sizeof(T);
	result.data   = &list[start];
	return result;
}

template<typename T>
inline Slice<T>
List_Slice(Slice<T>& slice, u32 start)
{
	Slice<T> result = {};
	result.length = slice.length - start;
	result.stride = slice.stride;
	result.data   = &slice[start];
	return result;
}

template<typename T, typename U>
inline Slice<U>
List_MemberSlice(List<T>& list, U T::* memberPtr)
{
	Slice<U> result = {};
	result.length = list.length;
	result.stride = sizeof(T);

	if (list.length > 0)
		result.data = &(list[0].*memberPtr);

	return result;
}

template<typename T, typename U, typename V>
inline Slice<V>
List_MemberSlice(List<T>& list, U T::* memberPtr1, V U::* memberPtr2)
{
	Slice<V> result = {};
	result.length = list.length;
	result.stride = sizeof(T);

	if (list.length > 0)
	{
		T& data0 = list[0];
		U& data1 = data0.*memberPtr1;
		V& data2 = data1.*memberPtr2;
		result.data = &data2;
	}

	return result;
}

template<typename T>
inline T&
List_GetLast(Slice<T>& list)
{
	Assert(list.length > 0);
	return list[list.length - 1];
}

template<typename T, typename U>
inline Slice<U>
Slice_MemberSlice(Slice<T>& slice, U T::* memberPtr)
{
	Slice<U> result = {};
	result.length = slice.length;
	result.stride = sizeof(T);

	if (slice.length > 0)
	{
		result.data   = &(slice[0].*memberPtr);
		result.stride = slice.stride;
	}

	return result;
}

// TODO: Change most of the API to take a Slice (have to deal with T deduction on the functions)
// TODO: This data structure needs to be reworked entirely. The API is all over the place and
// generally garbage.
// TODO: Settle on references or pointers
// TODO: Allow range-for usage
// TODO: Use placement new to avoid calling destructors on garbage objects?
// TODO: Use functions to zero empty slots. Use raw access to skip the zeroing. Use the function to
// fuzz unused slots with different values.
// TODO: Null object pattern and Refs are in tension with each other. There are two interesting
// cases: 1) Zero init data is 'invalid' in the sense it doesn't map to anything at all. Using zero
// init data is an error. 2) Zero init data is 'invalid' in the sense that it's not the intended
// value, but it maps to a default/null object and is valid to use.

#endif
