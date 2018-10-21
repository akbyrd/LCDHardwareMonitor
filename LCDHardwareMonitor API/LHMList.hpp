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
// - Does *not* support move semantics. Items are copy-assigned into spots.
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
	bool operator==(const ListRef& other) { return index == other.index; }
};

template<typename T>
struct List
{
	u32 length;
	u32 capacity;
	T*  data;

	using RefT = ListRef<T>;
	inline T& operator [](RefT r) { return data[r.index]; }
	//inline T& operator [](u32 i)  { return data[i]; }
	inline    operator T*()       { return data; }
	// TODO: Maybe we want to be checking length, not data?
	inline    operator b32()      { return data != nullptr; }
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
List_Append(List<T>& list, T& item)
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

template<typename T>
inline b32
List_AppendRange(List<T>& list, List<T> items)
{
	if (!List_Reserve(list, list.length + items.length))
		return false;

	memcpy(&list.data[list.length], items.data, items.length);

	return true;
}

template<typename T>
inline b32
List_Contains(List<T>& list, T* item)
{
	if (list.length == 0)
		return false;

	return item >= &list[0] && item < &list[list.length];
}

template<typename T>
inline b32
List_Contains(List<T>& list, T& item)
{
	if (list.length == 0)
		return false;

	// TODO: This returns false positives if the address in the range, but not an actual item
	return &item >= &list[0] && &item < &list[list.length];
}

// NOTE: I guess this only works if T has a custom equality operator?
template<typename T>
inline b32
List_ContainsValue(List<T>& list, T& item)
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
inline ListRef<T>
List_GetRef(List<T>& list, u32 index)
{
	UNUSED(list);
	ListRef<T> ref = {};
	ref.index = index;
	return ref;
}

template<typename T>
inline T*
List_GetLastPtr(List<T>& list)
{
	if (list.length == 0)
		return nullptr;

	return &list[list.length - 1];
}

template<typename T>
inline void
List_RemoveFast(List<T>& list, T& item)
{
	item = list[list.length - 1];
	list.length--;
}

template<typename T>
inline void
List_RemoveLast(List<T>& list)
{
	if (list.length > 0) {
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
inline List<T>
List_Duplicate(List<T>& list)
{
	List<T> duplicate = {};
	List_Reserve(duplicate, list.capacity);
	// TODO: Fix this
	//if (!duplicate) return nullptr;

	duplicate.length   = list.length;
	duplicate.capacity = list.capacity;

	if (duplicate.data && list.data)
		memcpy(duplicate.data, list.data, list.capacity);

	return duplicate;
}

template<typename T>
inline b32
List_Equal(List<T>& listA, List<T>& listB)
{
	if (listA.length != listB.length)
		return false;

	for (u32 i = 0; i < listA.length; i++)
	{
		if (listA[i] != listB[i])
			return false;
	}

	return true;
}

// TODO: This data structure needs to be reworked entirely. The API is all
// over the place and generally garbage.
// TODO: I bet we can make a pretty simple fixed-size list with the same
// interface (e.g. transparently change between them).
// TODO: Array and String slices?
// TODO: Settle on references or pointers
// TODO: Handle cases where this may fail
// TODO: Where should memory be included?
// TODO: Create can fail too! Yay!
// TODO: List_Shrink - if <=25% full shrink to 50%
// TODO: Allow foreach style usage
// TODO: Use placement new to avoid calling destructors on garbage objects?

#endif
