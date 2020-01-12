#ifndef LHM_BYTES
#define LHM_BYTES
namespace LCDHardwareMonitor {

// TODO: Create real types for Bytes & ByteSlice
// TODO: Allow conversion from List & Slice types
// TODO: Allow explicit cast to List & Slice types
using Bytes = List<u8>;
using ByteSlice = Slice<u8>;
using ScopedBytes = Scoped<List<u8>, List_Free<u8>>;

template<typename T>
void
Bytes_ReadObject(Bytes& bytes, u32 offset, T& object)
{
	T& src = (T&) bytes[offset];
	object = *src;
}

template<typename T>
b8
Bytes_WriteObject(Bytes& bytes, u32 offset, T& object)
{
	if (!List_Reserve(bytes, offset + sizeof(T)))
		return false;

	T& dst = (T&) bytes[offset];
	dst = object;
	bytes.length = Max(bytes.length, offset + sizeof(T));

	return true;
}

template<typename T>
b8
Bytes_FromObject(T& object, Bytes bytes)
{
	if (!List_Reserve(bytes, sizeof(T)))
		return false;

	T& dst = (T&) bytes.data;
	dst = object;
	bytes.length = sizeof(T);

	return true;
}

template<typename T>
ByteSlice
ByteSlice_FromObject(T& object)
{
	ByteSlice result = {};
	result.length = sizeof(T);
	result.stride = 1;
	result.data   = (u8*) &object;
	return result;
}

}
#endif
