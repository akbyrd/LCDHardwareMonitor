#pragma once

//TODO: Where should this be?
#include <memory>

template<typename T, i32 S>
inline i32 ArrayLength(const T(&arr)[S])
{
	return S;
}

template<typename T, i32 S>
inline i32 ArraySize(const T(&arr)[S])
{
	return S * sizeof(T);
}

/* NOTE: Elemnts beyond the length of the array are no zero initialized. It's
 * impossible to enfore this as the user may reduce the length at will and will
 * leave abandoned data. Copy assignment operators may access data in the
 * destination so we need to guaranteed it will not be garbage when this happens.
 * THe means with either zero the memory right before use, or we zero it all the
 * time and expect the caller not to make mistakes. Both of these are tempting,
 * but it seems too easy for the caller to screw up.
 */
//TODO: I bet we can make a pretty simple fixed-size list with the same interface (e.g. transparently change between them)
template<typename T>
struct List
{
	i32 length;
	i32 capacity;
	T*  data;

	inline T& operator [](i32 i) { return data[i]; }
	inline    operator T*()      { return data; }
	inline    operator bool()    { return data != nullptr; }
};

//TODO: This can fail too! Yay!
template<typename T>
inline List<T>
List_Create(i32 capacity = 4)
{
	List<T> list;

	list.length   = 0;
	list.data     = (T*) malloc(sizeof(T) * capacity);
	list.capacity = list.data ? capacity : 0;

	return list;
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
		i32 newCapacity = list.capacity ? 2*list.capacity : 4;
		T*  newItems    = (T*) realloc(list.data, sizeof(T) * list.capacity);

		if (!newItems)
			return false;

		list.capacity = newCapacity;
		list.data     = newItems;
	}

	return true;
}

//TODO: Maybe return references instead of pointers?
//TODO: Handle cases where this may fail
template<typename T>
inline T*
List_Append(List<T>& list, T& item)
{
	if (!List_Grow(list))
		return nullptr;

	T* element = &list.data[list.length++];
	*element = item;
	return element;
}

template<typename T>
inline T*
List_Append(List<T>& list)
{
	if (!List_Grow(list))
		return nullptr;

	T* element = &list.data[list.length++];
	return element;
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
inline T*
List_GetLastPtr(List<T>& list)
{
	if (list.length == 0)
		return nullptr;

	return &list[list.length - 1];
}

template<typename T>
inline void
List_RemoveLast(List<T>& list)
{
	if (list.length > 0)
		list.length--;
}

template<typename T>
inline void
List_Clear(List<T>& list)
{
	list.length = 0;
}

template<typename T>
inline List<T>
List_Duplicate(List<T>& list)
{
	List<T> duplicate = List_Create<T>(list.length);
	memcpy(duplicate.data, list.data, list.length);

	return duplicate;
}

template<typename T>
inline b32
List_Equal(List<T>& listA, List<T>& listB)
{
	if (listA.length != listB.length)
		return false;

	for (i32 i = 0; i < listA.length; i++)
	{
		if (listA[i] != listB[i])
			return false;
	}

	return true;
}