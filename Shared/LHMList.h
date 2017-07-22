#pragma once

//TODO: Where should this be?
#include <memory>

template<typename T, i32 S>
inline i32 ArrayCount(const T(&arr)[S])
{
	return S;
}

template<typename T, i32 S>
inline i32 ArraySize(const T(&arr)[S])
{
	return S * sizeof(T);
}

//TODO: I bet we can make a pretty simple fixed-size list with the same interface (e.g. transparently change between them)
template<typename T>
struct List
{
	i32 count;
	i32 capacity;
	T*  items;

	T& operator[](i32 index)
	{
		return items[index];
	}
};

//TODO: This can fail too! Yay!
template<typename T>
inline List<T>
List_Create(i32 capacity = 4)
{
	List<T> list;

	list.count    = 0;
	list.items    = (T*) malloc(sizeof(T) * capacity);
	list.capacity = list.items ? capacity : 0;

	return list;
}

template<typename T>
inline void
List_Free(List<T>& list)
{
	free(list.items);
	list = {};
}

template<typename T>
inline b32
List_Grow(List<T>& list)
{
	if (list.count == list.capacity)
	{
		i32 newCapacity = list.capacity ? 2*list.capacity : 4;
		T*  newItems    = (T*) realloc(list.items, sizeof(T) * list.capacity);

		if (!newItems)
			return false;

		list.capacity = newCapacity;
		list.items    = newItems;
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

	T* element = &list.items[list.count++];
	*element = item;
	return element;
}

template<typename T>
inline T*
List_Append(List<T>& list)
{
	if (!List_Grow(list))
		return nullptr;

	T* element = &list.items[list.count++];
	return element;
}

template<typename T>
inline b32
List_Contains(List<T>& list, T* item)
{
	if (list.count == 0)
		return false;

	return item >= &list[0] && item < &list[list.count];
}

template<typename T>
inline T*
List_GetLastPtr(List<T>& list)
{
	if (list.count == 0)
		return nullptr;

	return &list[list.count - 1];
}

template<typename T>
inline void
List_RemoveLast(List<T>& list)
{
	if (list.count > 0)
		list.count--;
}

template<typename T>
inline void
List_Clear(List<T>& list)
{
	list.count = 0;
}

template<typename T>
inline List<T>
List_Duplicate(List<T>& list)
{
	List<T> duplicate = List_Create<T>(list.count);
	memcpy(duplicate.items, list.items, list.count);

	return duplicate;
}

template<typename T>
inline b32
List_Equal(List<T>& listA, List<T>& listB)
{
	if (listA.count != listB.count)
		return false;

	for (i32 i = 0; i < listA.count; i++)
	{
		if (listA[i] != listB[i])
			return false;
	}

	return true;
}