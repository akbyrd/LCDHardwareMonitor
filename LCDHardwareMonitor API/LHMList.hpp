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
//   Append return a nullptr instead of a pointer to the new slot.

#include <memory>

template<typename T>
struct ListRef
{
	u32 index;
	b32 operator== (const ListRef& other) { return index == other.index; }
	b32 operator!= (const ListRef& other) { return index != other.index; }

	// TODO: Switch to 0 being a null asset everywhere
	static const ListRef<T> Null;
	operator b32() { return *this != Null; }
};

template<typename T>
const ListRef<T> ListRef<T>::Null = { u32Max };

template<typename T>
struct List
{
	u32 length;
	u32 capacity;
	T*  data;

	using RefT = ListRef<T>;
	inline T& operator[] (RefT r) { return data[r.index]; }
	inline T& operator[] (u32 i)  { return data[i]; }
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
inline void
List_Free(List<T>& list)
{
	free(list.data);
	list = {};
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

	T* slot = &list.data[list.length++];
	*slot = item;
	return slot;
}

template<typename T>
inline T*
List_Append(List<T>& list)
{
	if (!List_Grow(list))
		return nullptr;

	T* slot = &list.data[list.length++];
	return slot;
}

// List_AppendRange - See LHMSlice.hpp

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
inline ListRef<T>
List_FindFirst(List<T>& list, T item)
{
	for (u32 i = 0; i < list.length; i++)
		if (list.data[i] == item)
			return List_Get(list, i);

	return ListRef<T>::Null;
}

template<typename T>
inline ListRef<T>
List_FindLast(List<T>& list, T item)
{
	for (u32 i = list.length - 1; i >= 0; i--)
		if (list.data[i] == item)
			return List_Get(list, i);

	return ListRef<T>::Null;
}

template<typename T>
inline ListRef<T>
List_Get(List<T>& list, u32 index)
{
	UNUSED(list);
	ListRef<T> ref = {};
	ref.index = index;
	return ref;
}

template<typename T>
inline ListRef<T>
List_GetFirst(List<T>& list)
{
	return List_Get(list, 0);
}

template<typename T>
inline ListRef<T>
List_GetLast(List<T>& list)
{
	return List_Get(list, list.length - 1);
}

template<typename T>
inline T*
List_GetFirstPtr(List<T>& list)
{
	if (list.length == 0)
		return nullptr;

	return &list.data[0];
}

template<typename T>
inline T*
List_GetLastPtr(List<T>& list)
{
	if (list.length == 0)
		return nullptr;

	return &list.data[list.length - 1];
}

template<typename T>
inline b32
List_IsRefValid(List<T>& list, ListRef<T> ref)
{
	return ref.index < list.length;
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
inline void
List_Clear(List<T>& list)
{
	list.length = 0;
	memset(list.data, 0, sizeof(T) * list.capacity);
}

template<typename T>
inline b32
List_Duplicate(List<T>& list, List<T>& duplicate)
{
	List_Reserve(duplicate, list.capacity);
	if (!duplicate.data)
		return false;

	duplicate.length   = list.length;
	duplicate.capacity = list.capacity;

	if (duplicate.data && list.data)
		memcpy(duplicate.data, list.data, list.capacity);

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

using Bytes = List<u8>;

// TODO: This data structure needs to be reworked entirely. The API is all
// over the place and generally garbage.
// TODO: I bet we can make a pretty simple fixed-size list with the same
// interface (e.g. transparently change between them).
// TODO: Settle on references or pointers
// TODO: Handle cases where this may fail
// TODO: Where should memory be included?
// TODO: List_Shrink - if <=25% full shrink to 50%
// TODO: Allow foreach style usage
// TODO: Use placement new to avoid calling destructors on garbage objects?

#endif
