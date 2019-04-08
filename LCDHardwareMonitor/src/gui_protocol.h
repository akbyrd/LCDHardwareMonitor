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

// TODO: Use function overloading so we don't have to do this
template<typename T>
using SerializeItemFn = void(T&, ByteStream&);

template<typename T>
void Slice_Serialize(Slice<T>&, ByteStream&, SerializeItemFn<T>*);

template<typename T>
void
Primitive_Serialize(T& primitive, ByteStream& stream)
{
	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			break;

		case ByteStreamMode::Read:
			break;

		case ByteStreamMode::Write:
		{
			T* dst = (T*) &stream.bytes[stream.cursor];
			*dst = primitive;
			break;
		}
	}
	stream.cursor += sizeof(T);
}

void String_Serialize(StringSlice&, ByteStream&);
void Plugins_Serialize(Message::Plugins&, ByteStream&);
void Connect_Serialize(Message::Connect&, ByteStream&);
void PluginInfo_Serialize(PluginInfo&, ByteStream&);

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
		// TODO: Use function overloading so we don't have to do this
		SerializeItemFn<T>* serializeItem = nullptr;
		switch (T::Id)
		{
			default: Assert(false); break;

			// TODO: Janky as fuck
			case Connect::Id: serializeItem = (SerializeItemFn<T>*) Connect_Serialize; break;
			case Plugins::Id: serializeItem = (SerializeItemFn<T>*) Plugins_Serialize; break;
		}

		// TODO: Consider something like a Pointer_Serialize
		stream.mode = ByteStreamMode::Size;
		Primitive_Serialize(message, stream);
		serializeItem(message, stream);

		b32 success = List_Reserve(stream.bytes, stream.cursor);
		LOG_IF(!success, return false,
			Severity::Fatal, "Failed to allocate GUI message");
		stream.bytes.length = stream.cursor;

		message.header.id   = T::Id;
		message.header.size = stream.bytes.length;

		stream.mode   = ByteStreamMode::Write;
		stream.cursor = 0;
		Primitive_Serialize(message, stream);
		// TODO: Janky as fuck
		T* messageCopy = (T*) &stream.bytes[0];
		serializeItem(*messageCopy, stream);

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
					Primitive_Serialize(*connect, stream);
					Connect_Serialize(*connect, stream);
					break;
				}

				case Plugins::Id:
				{
					Plugins* plugins = (Plugins*) &bytes[0];
					Primitive_Serialize(*plugins, stream);
					Plugins_Serialize(*plugins, stream);
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

template<typename T>
void
Slice_Serialize(Slice<T>& slice, ByteStream& stream, SerializeItemFn<T>* serializeItem)
{
	T* data = (T*) &stream.bytes[stream.cursor];
	stream.cursor += List_SizeOf(slice);

	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			for (u32 i = 0; i < slice.length; i++)
				serializeItem(slice[i], stream);
			break;

		case ByteStreamMode::Read:
			slice.data = data;
			for (u32 i = 0; i < slice.length; i++)
				serializeItem(slice[i], stream);
			break;

		case ByteStreamMode::Write:
		{
			for (u32 i = 0; i < slice.length; i++)
			{
				data[i] = slice[i];
				serializeItem(slice[i], stream);
			}

			slice.stride = sizeof(T);
			slice.data   = nullptr;
			break;
		}
	}
}

void
String_Serialize(StringSlice& string, ByteStream& stream)
{
	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			stream.cursor += List_SizeOf(string);
			break;

		case ByteStreamMode::Read:
			string.data = (c8*) &stream.bytes[stream.cursor];
			stream.cursor += List_SizeOf(string);
			break;

		case ByteStreamMode::Write:
			Assert(string.stride == 1);
			c8* dst = (c8*) &stream.bytes[stream.cursor];
			memcpy(dst, string.data, string.length);
			stream.cursor += List_SizeOf(string);
			break;
	}
}

void
Plugins_Serialize(Message::Plugins& plugins, ByteStream& stream)
{
	//Primitive_Serialize(plugins.header, stream);
	Slice_Serialize(plugins.plugins, stream, PluginInfo_Serialize);
}

void
Connect_Serialize(Message::Connect& connect, ByteStream& stream)
{
	UNUSED(connect);
	UNUSED(stream);
	//Primitive_Serialize(connect.header, stream);
	//Primitive_Serialize(connect.version, stream);
}

void
PluginInfo_Serialize(PluginInfo& pluginInfo, ByteStream& stream)
{
	String_Serialize(pluginInfo.name, stream);
	String_Serialize(pluginInfo.author, stream);
	//Primitive_Serialize(pluginInfo.version, stream);
}

// TODO: Why do we get a duplicate set of messages when closing the GUI?
// TODO: Ensure pipe reconnects when closing and opening the GUI
// TODO: Ensure pipe reconnects when closing and opening the sim
// TODO: Code gen the actual serialize functions
// TODO: Refactor the success case out of switch statements
