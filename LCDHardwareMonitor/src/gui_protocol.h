namespace Message
{
	struct Header
	{
		u32 id;
		u32 size;
	};

	struct Null
	{
		static constexpr u32 Id = 0;
		Header header;
	};

	struct Connect
	{
		static constexpr u32 Id = IdOf<Connect>();
		Header header;

		u32 version;
	};

	struct Plugins
	{
		static constexpr u32 Id = IdOf<Plugins>();
		Header header;

		Slice<PluginInfo> plugins;
	};
};

u32 messageOrder[] = {
	Message::Connect::Id,
	Message::Plugins::Id,
	Message::Null::Id,
};

void
IncrementMessage(u32* currentMessageId)
{
	for (u32 i = 0; i < ArrayLength(messageOrder); i++)
	{
		if (messageOrder[i] == *currentMessageId)
		{
			*currentMessageId = messageOrder[i + 1];
			return;
		}
	}
	Assert(false);
}

template <typename T>
b32
SendMessage(Pipe* pipe, T& message, u32* currentMessageId)
{
	Assert(T::Id == *currentMessageId);

	message.header.id   = T::Id;
	message.header.size = sizeof(T);

	PipeResult result = Platform_WritePipe(pipe, message);
	switch (result)
	{
		default: Assert(false); break;

		case PipeResult::Success:
			IncrementMessage(currentMessageId);
			break;

		case PipeResult::TransientFailure:
			// Ignore failures, retry next frame
			break;

		case PipeResult::UnexpectedFailure:
			return false;
	}

	return true;
}

template <typename T>
b32
SendMessage(Pipe* pipe, T&, Bytes bytes, u32* currentMessageId)
{
	Assert(T::Id == *currentMessageId);

	using namespace Message;
	Header* header = (Header*) bytes.data;
	header->id   = T::Id;
	header->size = bytes.length;

	PipeResult result = Platform_WritePipe(pipe, bytes);
	switch (result)
	{
		default: Assert(false); break;

		case PipeResult::Success:
			IncrementMessage(currentMessageId);
			break;

		case PipeResult::TransientFailure:
			// Ignore failures, retry next frame
			break;

		case PipeResult::UnexpectedFailure:
			return false;
	}

	return true;
}

PipeResult
ReceiveMessage(Pipe* pipe, Bytes& bytes, u32* currentMessageId)
{
	using namespace Message;

	PipeResult result = Platform_ReadPipe(pipe, bytes);
	switch (result)
	{
		default: Assert(false); break;

		case PipeResult::Success:
		{
			if (bytes.length == 0) return PipeResult::TransientFailure;

			LOG_IF(bytes.length < sizeof(Header), return PipeResult::TransientFailure,
				Severity::Warning, "Corrupted message received");

			Header* header = (Header*) bytes.data;

			LOG_IF(*currentMessageId != header->id, return PipeResult::TransientFailure,
				Severity::Warning, "Unexpected message received");

			LOG_IF(bytes.length != header->size, return PipeResult::TransientFailure,
				Severity::Warning, "Truncated message received");

			IncrementMessage(currentMessageId);
			break;
		}

		case PipeResult::TransientFailure:
			// Ignore failures, retry next frame
			return PipeResult::TransientFailure;

		case PipeResult::UnexpectedFailure:
			return PipeResult::UnexpectedFailure;
	}

	return PipeResult::Success;
}

// TODO: Code gen the actual serialize functions
// TODO: Implement as template specialization?
// TODO: Provide an allocator for deserialize? (or just refer to the byte buffer!)
// TODO: Even better: just blit the entire struct, then walk all the pointers, append them, and patch up pointers

enum ByteStreamMode
{
	Null,
	Size,
	Read,
	Write,
	//TODO: Free?
};

struct ByteStream
{
	ByteStreamMode mode;
	u32            cursor;
	Bytes          bytes;
};

template<typename T>
using SerializeItemFn = b32(T&, ByteStream&);

template<typename T>
b32 Slice_Serialize(Slice<T>&, ByteStream&, SerializeItemFn<T>*);

template<typename T>
b32 Primitive_Serialize(T&, ByteStream&);

b32 String_Serialize(StringSlice&, ByteStream&);
b32 Plugins_Serialize(Message::Plugins&, ByteStream&);
b32 PluginInfo_Serialize(PluginInfo&, ByteStream&);

template<typename T>
b32
Slice_Serialize(Slice<T>& slice, ByteStream& stream, SerializeItemFn<T>* serializeItem)
{
	b32 success = true;
	success = success && Primitive_Serialize(slice.length, stream);
	success = success && Primitive_Serialize(slice.stride, stream);

	if (stream.mode == ByteStreamMode::Read)
	{
		// TODO: Would rather do this on write, but the code is awkward
		slice.stride = sizeof(T);
		// TODO: A Slice isn't supposed to have ownership of the data, but this one will
		slice.data   = (T*) malloc(slice.length * sizeof(T));
		success = slice.data != nullptr;
	}

	for (u32 i = 0; i < slice.length && success; i++)
		success = success && serializeItem(slice[i], stream);

	return success;
}

template<typename T>
b32
Primitive_Serialize(T& primitive, ByteStream& stream)
{
	b32 success;
	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			break;

		case ByteStreamMode::Read:
			Bytes_ReadObject(stream.bytes, stream.cursor, primitive);
			break;

		case ByteStreamMode::Write:
			success = Bytes_WriteObject(stream.bytes, stream.cursor, primitive);
			if (!success) return false;
			break;
	}

	stream.cursor += sizeof(T);
	return true;
}

b32
String_Serialize(StringSlice& string, ByteStream& stream)
{
	b32 success = true;
	success = success && Primitive_Serialize(string.length, stream);
	success = success && Primitive_Serialize(string.stride, stream);
	if (!success) return false;

	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			break;

		case ByteStreamMode::Read:
		{
			// TODO: A StringSlice isn't supposed to have ownership of a string, but this one will
			c8* src = (c8*) &stream.bytes[stream.cursor];
			c8* dst = (c8*) malloc(string.length);
			if (!dst) return false;
			memcpy(dst, src, string.length);
			string.data = dst;
			break;
		}

		case ByteStreamMode::Write:
		{
			u32 combinedLength = stream.bytes.length + string.length;
			success = List_Reserve(stream.bytes, combinedLength);
			if (!success) return false;

			c8* src = string.data;
			c8* dst = (c8*) &stream.bytes[stream.bytes.length];
			memcpy(dst, src, string.length);
			stream.bytes.length += string.length;
			break;
		}
	}

	stream.cursor += string.length;
	return true;
}

b32
Plugins_Serialize(Message::Plugins& plugins, ByteStream& stream)
{
	b32 success = true;
	success = success && Primitive_Serialize(plugins.header, stream);
	success = success && Slice_Serialize(plugins.plugins, stream, PluginInfo_Serialize);
	return success;
}

b32
PluginInfo_Serialize(PluginInfo& pluginInfo, ByteStream& stream)
{
	b32 success = true;
	success = success && String_Serialize(pluginInfo.name, stream);
	success = success && String_Serialize(pluginInfo.author, stream);
	success = success && Primitive_Serialize(pluginInfo.version, stream);
	return success;
}

// TODO: Why do we get a duplicate set of messages when closing the GUI?
// TODO: Ensure pipe reconnects when closing and opening the GUI
// TODO: Ensure pipe reconnects when closing and opening the sim
