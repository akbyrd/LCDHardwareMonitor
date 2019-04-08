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

	struct Sensors
	{
		static constexpr u32 Id = IdOf<Sensors>();
		Header header;

		// TODO: The raw strings here are a problem. I think we need to do the in-place serialize before this step
		Slice<Sensor> sensors;
	};
};

u32 messageOrder[] = {
	Message::Connect::Id,
	Message::Plugins::Id,
	Message::Null::Id,
};

enum ByteStreamMode
{
	Null,
	Size,
	Read,
	Write,
};

struct ByteStream
{
	ByteStreamMode mode;
	u32            cursor;
	Bytes          bytes;
};

// NOTE: There's one small gotcha with this serialization approach. We use
// Serialize(T&, ByteStream&) to support a pattern where you simply call Serialize for every member
// of your type and it will serialize properly as long as overloads exist for types that aren't
// blittable (e.g. contain pointers). Unfortunately, this function signature will match anything
// so if a necessary overload is missing we'll serialize the data anyway and it will fail somewhere
// on the receivers side when they try to dereference a pointer that came from the sender.

template<typename T>
void Serialize(T*&&, ByteStream&);

template<typename T>
void Serialize(T*&, ByteStream&);

template<typename T>
void Serialize(T&, ByteStream&);

template<typename T>
void Serialize(Slice<T>&, ByteStream&);

void Serialize(StringSlice&, ByteStream&);

void Serialize(Message::Plugins&, ByteStream&);
void Serialize(Message::Connect&, ByteStream&);
void Serialize(PluginInfo&, ByteStream&);

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
	using namespace Message;

	// Build the buffer
	ByteStream stream = {};
	defer { List_Free(stream.bytes); };
	{
		stream.mode = ByteStreamMode::Size;
		Serialize(&message, stream);

		b32 success = List_Reserve(stream.bytes, stream.cursor);
		LOG_IF(!success, return false,
			Severity::Fatal, "Failed to allocate GUI message");
		stream.bytes.length = stream.cursor;

		message.header.id   = T::Id;
		message.header.size = stream.bytes.length;

		stream.mode   = ByteStreamMode::Write;
		stream.cursor = 0;
		Serialize(&message, stream);

		Assert(stream.cursor == stream.bytes.length);
	}

	// Ship it off
	{
		PipeResult result = Platform_WritePipe(pipe, stream.bytes);
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
				Severity::Warning, "Incorrectly sized message received");

			ByteStream stream = {};
			stream.mode  = ByteStreamMode::Read;
			stream.bytes = bytes;

			switch (header->id)
			{
				default: Assert(false); break;

				case Connect::Id:
				{
					Connect* connect = (Connect*) &bytes[0];
					Serialize(connect, stream);
					break;
				}

				case Plugins::Id:
				{
					Plugins* plugins = (Plugins*) &bytes[0];
					Serialize(plugins, stream);
					break;
				}
			}

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

// TODO: This is a bit lame
template<typename T>
void Serialize(T*&& rpointer, ByteStream& stream)
{
	T* pointer = rpointer;
	Serialize(pointer, stream);
}

template<typename T>
void Serialize(T*& pointer, ByteStream& stream)
{
	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			break;

		case ByteStreamMode::Read:
		{
			// Point to the data appended to the stream
			T* data = (T*) &stream.bytes[stream.cursor];
			pointer = data;
			break;
		}

		case ByteStreamMode::Write:
		{
			// Copy the data into the stream
			T* data = (T*) &stream.bytes[stream.cursor];
			*data = *pointer;
			pointer = data;
			break;
		}
	}

	stream.cursor += sizeof(T);
	Serialize(*pointer, stream);
}

template<typename T>
void
Serialize(T& value, ByteStream& stream)
{
	// No-op
	UNUSED(value);
	UNUSED(stream);
}

template<typename T>
void
Serialize(Slice<T>& slice, ByteStream& stream)
{
	T* data = (T*) &stream.bytes[stream.cursor];
	stream.cursor += List_SizeOf(slice);

	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			for (u32 i = 0; i < slice.length; i++)
				Serialize(slice[i], stream);
			break;

		case ByteStreamMode::Read:
			slice.data = data;
			for (u32 i = 0; i < slice.length; i++)
				Serialize(slice[i], stream);
			break;

		case ByteStreamMode::Write:
		{
			for (u32 i = 0; i < slice.length; i++)
			{
				data[i] = slice[i];
				Serialize(slice[i], stream);
			}

			slice.stride = sizeof(T);
			slice.data   = nullptr;
			break;
		}
	}
}

void
Serialize(StringSlice& string, ByteStream& stream)
{
	c8* data = (c8*) &stream.bytes[stream.cursor];
	stream.cursor += List_SizeOf(string);

	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			break;

		case ByteStreamMode::Read:
			string.data = data;
			break;

		case ByteStreamMode::Write:
			Assert(string.stride == 1);
			memcpy(data, string.data, string.length);
			string.data = nullptr;
			break;
	}
}

void
Serialize(Message::Plugins& plugins, ByteStream& stream)
{
	Serialize(plugins.header, stream);
	Serialize(plugins.plugins, stream);
}

void
Serialize(Message::Connect& connect, ByteStream& stream)
{
	Serialize(connect.header, stream);
	Serialize(connect.version, stream);
}

void
Serialize(PluginInfo& pluginInfo, ByteStream& stream)
{
	Serialize(pluginInfo.name, stream);
	Serialize(pluginInfo.author, stream);
	Serialize(pluginInfo.version, stream);
}

// TODO: Why do we get a duplicate set of messages when closing the GUI?
// TODO: Ensure pipe reconnects when closing and opening the GUI
// TODO: Ensure pipe reconnects when closing and opening the sim
// TODO: Code gen the actual Serialize functions
// TODO: Refactor the success case out of switch statements
