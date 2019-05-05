namespace Message
{
	struct Header
	{
		u32 id;
		u32 index;
		u32 size;
	};

	struct Null
	{
		Header header;
	};

	struct Connect
	{
		Header header;
		u32    version;
		size   renderSurface;
		v2u    renderSize;
	};

	struct Plugins
	{
		Header header;

		Slice<SensorPluginRef> sensorPluginRefs;
		Slice<PluginInfo>      sensorPlugins;

		Slice<WidgetPluginRef> widgetPluginRefs;
		Slice<PluginInfo>      widgetPlugins;
	};

	struct Sensors
	{
		Header                 header;
		Slice<SensorPluginRef> sensorPluginRefs;
		Slice<List<Sensor>>    sensors;
	};
};

template <>
constexpr u32 IdOf<Message::Null> = 0;

enum struct ByteStreamMode
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

template <typename T>
b32
SerializeMessage(T& message, Bytes& bytes, u32 messageIndex)
{
	using namespace Message;

	ByteStream stream = {};
	defer { List_Free(stream.bytes); };

	stream.mode = ByteStreamMode::Size;
	Serialize(stream, &message);

	b32 success = List_Reserve(stream.bytes, stream.cursor);
	LOG_IF(!success, return false,
		Severity::Fatal, "Failed to allocate GUI message");
	stream.bytes.length = stream.cursor;

	message.header.id    = IdOf<T>;
	message.header.index = messageIndex;
	message.header.size  = stream.bytes.length;

	stream.mode   = ByteStreamMode::Write;
	stream.cursor = 0;
	Serialize(stream, &message);

	Assert(stream.cursor == stream.bytes.length);

	bytes = stream.bytes;
	stream.bytes = {};
	return true;
}

template <typename T>
void
DeserializeMessage(Bytes& bytes)
{
	using namespace Message;

	ByteStream stream = {};
	stream.mode  = ByteStreamMode::Read;
	stream.bytes = bytes;

	Serialize(stream, (T*) &bytes[0]);
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
	Serialize(stream, sensor.format);
	Serialize(stream, sensor.value);
}

// TODO: Why do we get a duplicate set of messages when closing the GUI?
// TODO: Code gen the actual Serialize functions
