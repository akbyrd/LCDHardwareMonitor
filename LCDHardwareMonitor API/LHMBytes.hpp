#ifndef LHM_BYTES
#define LHM_BYTES

// TODO: Create real types for Bytes & ByteSlice
// TODO: Allow conversion from List & Slice types
// TODO: Allow explicit cast to List & Slice types
using Bytes = List<u8>;
using ByteSlice = Slice<u8>;

template<typename T>
void
Bytes_ReadObject(Bytes& bytes, u32 offset, T& object)
{
	T* src = (T*) (bytes.data + offset);
	object = *src;
}

template<typename T>
b32
Bytes_WriteObject(Bytes& bytes, u32 offset, T& object)
{
	if (!List_Reserve(bytes, offset + sizeof(T)))
		return false;

	T* dest = (T*) (bytes.data + offset);
	*dest = object;
	bytes.length = Max(bytes.length, offset + sizeof(T));

	return true;
}

template<typename T>
b32
Bytes_FromObject(T* object, Bytes bytes)
{
	if (bytes.capacity == 0)
	{
		if (!List_Grow(bytes))
			return false;
	}

	bytes.length = sizeof(T);
	bytes.stride = 1;

	T* dest = List_Append(bytes);
	*dest = *object;
	return true;
}

template<typename T>
ByteSlice
ByteSlice_FromObject(T* object)
{
	ByteSlice result = {};
	result.length = sizeof(T);
	result.stride = 1;
	result.data   = (u8*) object;
	return result;
}

#endif
