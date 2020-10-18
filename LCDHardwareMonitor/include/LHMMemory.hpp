#ifndef LHM_MEMORY
#define LHM_MEMORY

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
