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

		u32  version;
		size renderSurface;
		v2u  renderSize;
	};

	struct Plugins
	{
		static constexpr u32 Id = IdOf<Plugins>();
		Header header;

		Slice<SensorPluginRef> sensorPluginRefs;
		Slice<PluginInfo>      sensorPlugins;

		Slice<WidgetPluginRef> widgetPluginRefs;
		Slice<PluginInfo>      widgetPlugins;
	};

	struct Sensors
	{
		static constexpr u32 Id = IdOf<Sensors>();
		Header header;

		Slice<SensorPluginRef> sensorPluginRefs;
		Slice<List<Sensor>>    sensors;
	};
};

u32 messageOrder[] = {
	Message::Connect::Id,
	Message::Plugins::Id,
	Message::Sensors::Id,
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
// Serialize(ByteStream&, T&) to support a pattern where you simply call Serialize for every member
// of your type and it will serialize properly as long as overloads exist for types that aren't
// blittable (e.g. contain pointers). Unfortunately, this function signature will match anything
// so if a necessary overload is missing we'll serialize the data anyway and it will fail somewhere
// on the receivers side when they try to dereference a pointer that came from the sender.

template<typename T>
void Serialize(ByteStream&, T*&&);

template<typename T>
void Serialize(ByteStream&, T*&);

template<typename T>
void Serialize(ByteStream&, T&);

template<typename T>
void Serialize(ByteStream&, List<T>&);

template<typename T>
void Serialize(ByteStream&, Slice<T>&);

void Serialize(ByteStream&, StringSlice&);

void Serialize(ByteStream&, Message::Plugins&);
void Serialize(ByteStream&, Message::Connect&);
void Serialize(ByteStream&, Message::Sensors&);
void Serialize(ByteStream&, PluginInfo&);
void Serialize(ByteStream&, Sensor&);

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
		Serialize(stream, &message);

		b32 success = List_Reserve(stream.bytes, stream.cursor);
		LOG_IF(!success, return false,
			Severity::Fatal, "Failed to allocate GUI message");
		stream.bytes.length = stream.cursor;

		message.header.id   = T::Id;
		message.header.size = stream.bytes.length;

		stream.mode   = ByteStreamMode::Write;
		stream.cursor = 0;
		Serialize(stream, &message);

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

b32
ReceiveMessage(Pipe* pipe, Bytes& bytes, u32* currentMessageId)
{
	using namespace Message;

	PipeResult result = Platform_ReadPipe(pipe, bytes);
	switch (result)
	{
		default: Assert(false); break;

		case PipeResult::Success:
			break;

		case PipeResult::TransientFailure:
			// Ignore failures, retry next frame
			return true;

		case PipeResult::UnexpectedFailure:
			return false;
	}

	if (bytes.length == 0) return true;

	LOG_IF(bytes.length < sizeof(Header), return true,
		Severity::Warning, "Corrupted message received");

	Header* header = (Header*) bytes.data;

	if (currentMessageId)
	{
		LOG_IF(*currentMessageId != header->id, return true,
			Severity::Warning, "Unexpected message received");
	}

	LOG_IF(bytes.length != header->size, return true,
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
			Serialize(stream, connect);
			break;
		}

		case Plugins::Id:
		{
			Plugins* plugins = (Plugins*) &bytes[0];
			Serialize(stream, plugins);
			break;
		}

		case Sensors::Id:
		{
			Sensors* sensors = (Sensors*) &bytes[0];
			Serialize(stream, sensors);
			break;
		}
	}

	if (currentMessageId)
		IncrementMessage(currentMessageId);
	return true;
}

// TODO: This is a bit lame
template<typename T>
void
Serialize(ByteStream& stream, T*&& rpointer)
{
	T* pointer = rpointer;
	Serialize(stream, pointer);
}

template<typename T>
void
Serialize(ByteStream& stream, T*& pointer)
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
	Serialize(stream, *pointer);
}

template<typename T>
void
Serialize(ByteStream& stream, T& value)
{
	// No-op
	UNUSED(value);
	UNUSED(stream);
}

template<typename T>
void
Serialize(ByteStream& stream, List<T>& list)
{
	T* data = (T*) &stream.bytes[stream.cursor];
	stream.cursor += List_SizeOf(list);

	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			for (u32 i = 0; i < list.length; i++)
				Serialize(stream, list[i]);
			break;

		case ByteStreamMode::Read:
			list.data = data;
			for (u32 i = 0; i < list.length; i++)
				Serialize(stream, list[i]);
			break;

		case ByteStreamMode::Write:
		{
			memcpy(data, list.data, List_SizeOf(list));
			for (u32 i = 0; i < list.length; i++)
				Serialize(stream, list[i]);

			list.capacity = list.length;
			list.data     = nullptr;
			break;
		}
	}
}

template<typename T>
void
Serialize(ByteStream& stream, Slice<T>& slice)
{
	T* data = (T*) &stream.bytes[stream.cursor];
	stream.cursor += List_SizeOf(slice);

	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			for (u32 i = 0; i < slice.length; i++)
				Serialize(stream, slice[i]);
			break;

		case ByteStreamMode::Read:
			slice.data = data;
			for (u32 i = 0; i < slice.length; i++)
				Serialize(stream, slice[i]);
			break;

		case ByteStreamMode::Write:
		{
			for (u32 i = 0; i < slice.length; i++)
				data[i] = slice[i];

			slice.data   = data;
			slice.stride = sizeof(T);

			for (u32 i = 0; i < slice.length; i++)
				Serialize(stream, slice[i]);

			// DEBUG: Easier to inspect
			//slice.data   = nullptr;
			break;
		}
	}
}

void
Serialize(ByteStream& stream, StringSlice& string)
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
Serialize(ByteStream& stream, Message::Connect& connect)
{
	Serialize(stream, connect.header);
	Serialize(stream, connect.version);
	Serialize(stream, connect.renderSurface);
	Serialize(stream, connect.renderSize);
}

void
Serialize(ByteStream& stream, Message::Plugins& plugins)
{
	Serialize(stream, plugins.header);
	Serialize(stream, plugins.sensorPlugins);
	Serialize(stream, plugins.sensorPluginRefs);
	Serialize(stream, plugins.widgetPlugins);
	Serialize(stream, plugins.widgetPluginRefs);
}

void
Serialize(ByteStream& stream, PluginInfo& pluginInfo)
{
	Serialize(stream, pluginInfo.name);
	Serialize(stream, pluginInfo.author);
	Serialize(stream, pluginInfo.version);
}

void
Serialize(ByteStream& stream, Message::Sensors& sensors)
{
	Serialize(stream, sensors.header);
	Serialize(stream, sensors.sensorPluginRefs);
	Serialize(stream, sensors.sensors);
}

void
Serialize(ByteStream& stream, Sensor& sensor)
{
	Serialize(stream, sensor.name);
	Serialize(stream, sensor.identifier);
	Serialize(stream, sensor.string);
	Serialize(stream, sensor.value);
	Serialize(stream, sensor.minValue);
	Serialize(stream, sensor.maxValue);
}

// TODO: Why do we get a duplicate set of messages when closing the GUI?
// TODO: Ensure pipe reconnects when closing and opening the GUI
// TODO: Ensure pipe reconnects when closing and opening the sim
// TODO: Code gen the actual Serialize functions
