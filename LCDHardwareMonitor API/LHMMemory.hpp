#ifndef LHM_MEMORY
#define LHM_MEMORY

struct AllocationHeader
{
	void* alloc;
	// TODO: Stack trace?
};

template<typename T>
T*
AlignUp(T* pointer)
{
	void* memory = pointer;
	u32 mod = memory % alignof(T);
	return (T*) (memory + (!!mod * (alignof(T) - mod)));
}

//struct HeapAllocator {};
//struct StackAllocator {};
struct FixedAllocator
{
	void* tail;
	void* memory;
	u32   capacity;

	template<typename T>
	T&
	ValueFromHeader(AllocationHeader& header)
	{
		T* value = (T*) ((void*) &header + sizeof(AllocationHeader));
		value = AlignUp(value);
		return *value;
	}

	template<typename T>
	AllocationHeader*
	HeaderFromValue(T* value);

	template<typename T>
	T&
	Alloc()
	{
		// TODO: Is dereferencing unowned memory without reading/writing allowed?
		AllocationHeader& header = (AllocationHeader&) *tail;
		T& value = ValueFromHeader(header);
		tail = &value + sizeof(T);
		Assert(tail < memory + capacity);
		header.alloc = &value;
		return value;
	}

	template<typename T>
	T&
	Alloc(u32 count)
	{
		Assert(count);
		T& value = Alloc<T>();
		tail += (count - 1) * sizeof(T);
		Assert(tail < memory + capacity);
		return value;
	}

	template<typename T>
	T* Free(T& value);

	void
	Reserve(u32 bytes)
	{
		Assert(!memory);
		memory   = malloc(bytes);
		capacity = bytes;
		tail     = memory;
	}
};

//struct LoggingContext {};

struct GlobalContext
{
	//HeapAllocator  heap           = {};
	//StackAllocator stack          = {};
	FixedAllocator loggingReserve = {};
	//LoggingContext logging        = {};
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

void* Free(void* memory)
{
	free(memory);
	return nullptr;
}

#endif
