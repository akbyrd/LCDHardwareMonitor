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
struct ListRefBase
{
	u32 index;
};

template<typename T>
struct ListRef : ListRefBase
{
	static const ListRef<T> Null;
	operator b32() { return *this != Null; }
};

template<typename T>
const ListRef<T> ListRef<T>::Null = {};

template<typename T>
inline b32 operator== (ListRef<T> lhs, ListRef<T> rhs) { return lhs.index == rhs.index; }

template<typename T>
inline b32 operator!= (ListRef<T> lhs, ListRef<T> rhs) { return lhs.index != rhs.index; }

template<typename T>
struct List
{
	u32 length;
	u32 capacity;
	T*  data;

	using RefT = ListRef<T>;
	inline T& operator[] (RefT r) { return data[r.index - 1]; }
	inline T& operator[] (u32 i)  { return data[i]; }
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

	                                 Slice()                                   { length = 0;                    stride = sizeof(T);            data = nullptr;            }
	template<typename U>             Slice(u32 _length, U* _data)              { length = _length;              stride = sizeof(U);            data = _data;              }
	template<typename U>             Slice(u32 _length, u32 _stride, U* _data) { length = _length;              stride = _stride;              data = _data;              }
	template<typename U>             Slice(const List<U>& list)                { length = list.length;          stride = sizeof(U);            data = list.data;          }
	template<typename U>             Slice(const Slice<U>& slice)              { length = slice.length;         stride = slice.stride;         data = slice.data;         }
	template<typename U>             Slice(const U& element)                   { length = 1;                    stride = sizeof(U);            data = (U*) &element;      }
	template<typename U, u32 Length> Slice(const U(&arr)[Length])              { length = Length;               stride = sizeof(U);            data = (U*) arr;           }
	//template<typename U>             Slice(ScopedList<U>& list)                { length = list.resource.length; stride = list.resource.stride; data = list.resource.data; }

	operator Slice<Slice<T>>() { return { 1, sizeof(Slice<T>), this }; }

	using RefT = ListRef<T>;
	inline T& operator[] (RefT r) { return (T&) ((u8*) data)[stride*(r.index - 1)]; }
	inline T& operator[] (u32 i)  { return (T&) ((u8*) data)[stride*i]; }
};

template<typename T>
inline b32
List_Reserve(List<T>& list, u32 capacity)
{
	if (list.capacity < capacity)
	{
		u64 totalSize = sizeof(T) * capacity;
		u64 emptySize = sizeof(T) * (capacity - list.length);

		T* data = (T*) realloc(list.data, (size) totalSize);
		if (!data) return false;

		list.capacity = capacity;
		list.data     = data;
		memset(&list.data[list.length], 0, (size) emptySize);
	}

	return true;
}

template<typename T>
inline b32
List_Grow(List<T>& list)
{
	if (list.length == list.capacity)
	{
		u32 capacity  = list.capacity ? 2*list.capacity : 4;
		u64 totalSize = sizeof(T) * capacity;
		u64 emptySize = sizeof(T) * (capacity - list.length);

		T* data = (T*) realloc(list.data, (size) totalSize);
		if (!data) return false;

		list.capacity = capacity;
		list.data     = data;
		memset(&list.data[list.length], 0, (size) emptySize);
	}

	return true;
}

template<typename T>
inline T*
List_Append(List<T>& list, T item)
{
	if (!List_Grow(list))
		return nullptr;

	T& slot = list.data[list.length++];
	slot = item;
	return &slot;
}

template<typename T>
inline T*
List_Append(List<T>& list)
{
	if (!List_Grow(list))
		return nullptr;

	T& slot = list.data[list.length++];
	return &slot;
}

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
inline void
List_Clear(List<T>& list)
{
	list.length = 0;
	memset(list.data, 0, sizeof(T) * list.capacity);
}

template<typename T>
inline b32
List_Contains(List<T>& list, T* item)
{
	if (list.length == 0)
		return false;

	// TODO: This returns false positives if the address in the range, but not an actual item
	b32 result = item >= &list.data[0] && item < &list.data[list.length];
	return result;
}

template<typename T>
inline b32
List_Contains(List<T>& list, T& item)
{
	b32 result = List_Contains(list, &item);
	return result;
}

template<typename T>
inline b32
List_ContainsValue(List<T>& list, T item)
{
	if (list.length == 0)
		return false;

	for (u32 i = 0; i < list.length; i++)
	{
		if (list.data[i] == item)
			return true;
	}
	return false;
}

template<typename T>
inline b32
List_Duplicate(Slice<T>& slice, List<T>& duplicate)
{
	if (!List_Reserve(duplicate, slice.length))
		return false;

	duplicate.length   = slice.length;
	duplicate.capacity = slice.length;

	for (u32 i = 0; i < slice.length; i++)
		duplicate[i] = slice[i];

	return true;
}

template<typename T>
inline b32
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
		if (list.data[i] == item)
			return List_GetRef(list, i);

	return ListRef<T>::Null;
}

template<typename T>
inline ListRef<T>
List_FindLast(List<T>& list, T item)
{
	for (u32 i = list.length; i > 0; i--)
		if (list.data[i - 1] == item)
			return List_GetRef(list, i);

	return ListRef<T>::Null;
}

template<typename T>
inline void
List_Free(List<T>& list)
{
	free(list.data);
	list = {};
}

template<typename T>
inline T&
List_Get(List<T>& list, u32 index)
{
	return list.data[index];
}

template<typename T>
inline T&
List_GetFirst(List<T>& list)
{
	// TODO: Decide what to do here
	//Assert(list.length > 0);
	return list.data[0];
}

template<typename T>
inline T&
List_GetLast(List<T>& list)
{
	//Assert(list.length > 0);
	return list.data[list.length - 1];
}

template<typename T>
inline ListRef<T>
List_GetRef(List<T>& list, u32 index)
{
	UNUSED(list);
	//Assert(list.length > 0);
	ListRef<T> ref = {};
	ref.index = index + 1;
	return ref;
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
inline T*
List_GetFirstPtr(List<T>& list)
{
	if (list.length == 0)
		return &list.data[0];

	return &list.data[0];
}

template<typename T>
inline T*
List_GetLastPtr(List<T>& list)
{
	if (list.length == 0)
		return &list.data[0];

	return &list.data[list.length - 1];
}

template<typename T>
inline u32
List_GetRemaining(List<T>& list)
{
	return list.capacity - list.length;
}

template<typename T>
inline b32
List_IsRefValid(List<T>& list, ListRef<T> ref)
{
	b32 result = (ref.index - 1) < list.length;
	return result;
}

template<typename T>
inline void
List_RemoveFast(List<T>& list, T& item)
{
	Assert(List_Contains(list, &item));
	item = list.data[list.length - 1];
	list.length--;
}

template<typename T>
inline void
List_RemoveLast(List<T>& list)
{
	if (list.length > 0)
	{
		list.length--;
		memset(&list.data[list.length], 0, sizeof(T));
	}
}

template<typename T>
inline b32
List_Shrink(List<T>& list)
{
	u32 capacity = list.capacity / 2;
	if (list.length < capacity)
	{
		u64 totalSize = sizeof(T) * capacity;
		T* data = (T*) realloc(list.data, (size) totalSize);
		if (!data) return false;

		list.capacity = capacity;
		list.data     = data;
	}

	return true;
}

// TODO: Consider renaming to List_SizeOfData
template<typename T>
inline size
List_SizeOf(List<T>& list)
{
	size result = list.length * sizeof(T);
	return result;
}

template<typename T>
inline size
List_SizeOf(Slice<T>& slice)
{
	size result = slice.length * sizeof(T);
	return result;
}

template<typename T>
inline size
List_SizeOfRemaining(List<T>& list)
{
	size result = (list.capacity - list.length) * sizeof(T);
	return result;
}

template<typename T>
inline Slice<T>
List_Slice(Slice<T>& slice, u32 start)
{
	Slice<T> result = {};
	result.length = slice.length - start;
	result.stride = slice.stride;
	result.data   = &slice.data[start];
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
		result.data = &(list.data[0].*memberPtr);

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
	return list.data[list.length - 1];
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
		result.data   = &(slice.data[0].*memberPtr);
		result.stride = slice.stride;
	}

	return result;
}

// TODO: Change most of the API to take a Slice (have to deal with T deduction on the functions)
// TODO: This data structure needs to be reworked entirely. The API is all over the place and
// generally garbage.
// TODO: Settle on references or pointers
// TODO: Where should memory be included?
// TODO: Allow range-for usage
// TODO: Use placement new to avoid calling destructors on garbage objects?

#endif
