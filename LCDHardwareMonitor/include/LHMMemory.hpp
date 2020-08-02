#ifndef LHM_MEMORY
#define LHM_MEMORY

template<typename T>
u8*
AlignUp(u8* address)
{
	u8 mod = (size) address % alignof(T);
	address += !!mod * (alignof(T) - mod);
	return address;
}

template <typename T>
u8*
AlignDown(u8* address)
{
	u8 mod = (size) address % alignof(T);
	address -= mod;
	return address;
}

u32
RelativeOffset(u8* from, u8* to)
{
	u32 offset = (u32) (to - from);
	return offset;
}

struct FixedAllocatorHeader
{
	u16 offsetToValue;
	u16 offsetFromPrevTail;
};

struct FixedAllocator
{
	using Header = FixedAllocatorHeader;

	u8* memory;
	u8* tail;
	u32 capacity;

	template<typename T>
	T*
	ValueFromTail(u8* tail)
	{
		tail = AlignUp<Header>(tail);
		tail += sizeof(Header);
		u8* value = AlignUp<T>(tail);
		return (T*) value;
	}

	template<typename T>
	Header*
	HeaderFromValue(T* value)
	{
		u8* header = ((u8*) value - sizeof(Header));
		header = AlignDown<Header>(header);
		return (Header*) header;
	}

	template<typename T>
	T&
	Alloc(u32 count)
	{
		Assert(memory);
		Assert(count);
		static_assert(alignof(T) <= sizeof(T));
		static_assert(sizeof(T) % alignof(T) == 0);

		// NOTE: We're assuming the alignment of a type is never greater than the size of the type and
		// that the size of a type is always a multiple of its alignment. I think these things are
		// always true, but there are asserts above in case they aren't.

		T* value = ValueFromTail<T>(tail);
		Header* header = HeaderFromValue(value);

		u32 offset = RelativeOffset(memory, (u8*) value);
		u32 size = count * sizeof(T);
		Assert(capacity - offset >= size);

		*header = {};
		header->offsetToValue      = (u16) RelativeOffset((u8*) header, (u8*) value);
		header->offsetFromPrevTail = (u16) RelativeOffset(tail, (u8*) header);

		tail = (u8*) value + size;

		return *value;
	}

	template<typename T>
	T&
	Alloc()
	{
		T& value = Alloc<T>(1);
		return value;
	}

	template<typename T>
	void Free(T& value)
	{
		Assert(memory);
		Assert(tail != memory);
		Assert((u8*) &value >= memory);
		Assert((u8*) &value <  memory + capacity);

		Header* header = HeaderFromValue(&value);
		tail = (u8*) header - header->offsetFromPrevTail;
	}

	void
	Reserve(u32 bytes)
	{
		// TODO: Remove forward declaration
		void* AllocChecked(size);

		Assert(!memory);
		memory     = (u8*) AllocChecked(bytes);
		capacity   = bytes;
		tail       = memory;
	}

	void
	Release()
	{
		Assert(memory);

		// TODO: Remove forward declaration
		void Free(void*);

		Free(memory);
		*this = {};
	}
};

void OutOfMemory()
{
	// TODO: Logging
	exit(-1);
}

void* AllocChecked(size bytes)
{
	void* memory = malloc(bytes);
	if (!memory) OutOfMemory();
	return memory;
}

void* AllocUnchecked(size bytes)
{
	void* memory = malloc(bytes);
	return memory;
}

void* ReallocChecked(void* memory, size bytes)
{
	memory = realloc(memory, bytes);
	if (!memory) OutOfMemory();
	return memory;
}

void* ReallocUnchecked(void* memory, size bytes)
{
	memory = realloc(memory, bytes);
	return memory;
}

void Free(void* memory)
{
	// TODO: Should we assert redundant frees?
	//Assert(memory);
	free(memory);
}

#endif

// TODO: Test AlignUp/Down thoroughly
// TODO: Magic bytes in AllocationHeader (type and id)
// TODO: Should we zero the memory?
// TODO: PoolAllocator
// TODO: StackAllocator
// TODO: HeapAllocator
// TODO: LoggingContext?

// TODO: Is a global context useful?
#if false
struct GlobalContext
{
	//HeapAllocator  heap           = {};
	//StackAllocator stack          = {};
	FixedAllocator loggingReserve = {};
	//LoggingContext logging        = {};
};
#endif
