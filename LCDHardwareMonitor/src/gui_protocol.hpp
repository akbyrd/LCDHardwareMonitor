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

	// Sim -> GUI
	struct Connect
	{
		Header header;
		u32    version;
		size   renderSurface;
		v2u    renderSize;
	};

	struct Disconnect
	{
		Header header;
	};

	// TODO: PluginsRemoved
	struct PluginsAdded
	{
		Header             header;
		PluginKind         kind; // TODO: Remove
		Slice<PluginRef>   refs;
		Slice<PluginInfo>  infos;
	};

	struct PluginStatesChanged
	{
		Header                 header;
		PluginKind             kind; // TODO: Remove
		Slice<PluginRef>       refs;
		Slice<PluginLoadState> loadStates;
	};

	// TODO: SensorsRemoved
	struct SensorsAdded
	{
		Header                 header;
		Slice<SensorPluginRef> pluginRefs;
		Slice<List<Sensor>>    sensors;
	};

	// TODO: WidgetDescsRemoved
	struct WidgetDescsAdded
	{
		Header                   header;
		Slice<WidgetPluginRef>   pluginRefs;
		Slice<Slice<WidgetDesc>> descs;
	};

	// GUI -> Sim
	struct TerminateSimulation
	{
		Header header;
	};

	struct MouseMove
	{
		Header header;
		v2i pos;
	};

	enum struct ButtonState
	{
		Null,
		Down,
		Up,
	};
	b8 operator!(ButtonState state) { return state == ButtonState::Null; }

	enum struct MouseButton
	{
		Null,
		Left,
		Right,
		Middle,
	};
	b8 operator!(MouseButton button) { return button == MouseButton::Null; }

	struct MouseButtonChange
	{
		Header header;
		v2i pos;
		MouseButton button;
		ButtonState state;
	};

	struct SetPluginLoadStates
	{
		Header                 header;
		PluginKind             kind; // TODO: Remove
		Slice<PluginRef>       refs;
		Slice<PluginLoadState> loadStates;
	};
}

template<>
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

struct ConnectionState
{
	Pipe        pipe;
	u32         sendIndex;
	u32         recvIndex;
	List<Bytes> queue; // TODO: Ring buffer
	u32         queueIndex;
	b8          failure;
};

void
Connection_Teardown(ConnectionState& con)
{
	Platform_DestroyPipe(con.pipe);

	for (u32 i = 0; i < con.queue.capacity; i++)
		List_Free(con.queue[i]);
	List_Free(con.queue);
}

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

// TODO: String?
void Serialize(ByteStream&, StringView&);

void Serialize(ByteStream&, Message::PluginsAdded&);
void Serialize(ByteStream&, Message::PluginStatesChanged&);
void Serialize(ByteStream&, Message::SetPluginLoadStates&);
void Serialize(ByteStream&, Message::SensorsAdded&);
void Serialize(ByteStream&, Message::WidgetDescsAdded&);
void Serialize(ByteStream&, PluginInfo&);
void Serialize(ByteStream&, Sensor&);

inline b8
MessageTimeLeft(i64 startTicks)
{
	return Platform_GetElapsedMilliseconds(startTicks) < 8;
}

template<typename T>
b8
SerializeMessage(Bytes& bytes, T& message, u32 messageIndex)
{
	using namespace Message;

	ByteStream stream = {};
	defer { List_Free(stream.bytes); };

	stream.mode = ByteStreamMode::Size;
	Serialize(stream, &message);

	b8 success = List_Reserve(stream.bytes, stream.cursor);
	LOG_IF(!success, return false,
		Severity::Fatal, "Failed to allocate message");
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

template<typename T>
void
DeserializeMessage(Bytes& bytes)
{
	using namespace Message;

	ByteStream stream = {};
	stream.mode  = ByteStreamMode::Read;
	stream.bytes = bytes;

	Serialize(stream, (T*) bytes.data);
}

b8
HandleMessageResult(ConnectionState& con, b8 success)
{
	if (!success)
	{
		con.failure = true;

		PipeResult result = Platform_DisconnectPipe(con.pipe);
		LOG_IF(result != PipeResult::Success, IGNORE,
			Severity::Error, "Failed to disconnect pipe during message failure");
	}

	return success;
}

PipeResult
HandleMessageResult(ConnectionState& con, PipeResult result)
{
	switch (result)
	{
		default: Assert(false); break;

		case PipeResult::Success: break;
		case PipeResult::TransientFailure: break;

		case PipeResult::UnexpectedFailure:
			HandleMessageResult(con, false);
			break;
	}

	return result;
}

b8
QueueMessage(ConnectionState& con, Bytes& bytes)
{
	Bytes* result = List_Append(con.queue, bytes);
	LOG_IF(!result, return false,
		Severity::Warning, "Failed to allocate message queue space");

	con.sendIndex++;
	return true;
}

template<typename T>
b8
SerializeAndQueueMessage(ConnectionState& con, T& message)
{
	b8 success;

	Bytes bytes = {};
	auto cleanupGuard = guard { List_Free(bytes); };

	success = SerializeMessage(bytes, message, con.sendIndex);
	LOG_IF(!success, return HandleMessageResult(con, false),
		Severity::Fatal, "Failed to serialize message");

	success = QueueMessage(con, bytes);
	LOG_IF(!success, return HandleMessageResult(con, false),
		Severity::Warning, "Failed to queue message");

	cleanupGuard.dismiss = true;
	return true;
}

b8
SendMessage(ConnectionState& con)
{
	if (con.failure) return false;

	// Nothing to send
	if (con.queueIndex == con.queue.length) return false;

	Bytes bytes = con.queue[con.queueIndex];

	PipeResult result = Platform_WritePipe(con.pipe, bytes);
	HandleMessageResult(con, result);
	if (result != PipeResult::Success) return false;

	con.queueIndex++;
	if (con.queueIndex == con.queue.length)
	{
		for (u32 i = 0; i < con.queue.length; i++)
			con.queue[i].length = 0;
		con.queue.length = 0;
		con.queueIndex = 0;
	}

	return true;
}

b8
ReceiveMessage(ConnectionState& con, Bytes& bytes)
{
	if (con.failure) return false;

	PipeResult result = Platform_ReadPipe(con.pipe, bytes);
	HandleMessageResult(con, result);
	if (result != PipeResult::Success) return false;
	if (bytes.length == 0) return false;

	using namespace Message;
	Header& header = (Header&) bytes[0];

	LOG_IF(bytes.length < sizeof(Header), return HandleMessageResult(con, false),
		Severity::Warning, "Corrupted message received");

	LOG_IF(bytes.length != header.size, return HandleMessageResult(con, false),
		Severity::Warning, "Incorrectly sized message received");

	LOG_IF(header.index != con.recvIndex, return HandleMessageResult(con, false),
		Severity::Warning, "Unexpected message received");

	con.recvIndex++;
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
Serialize(ByteStream& stream, String& string)
{
	c8* data = (c8*) &stream.bytes[stream.cursor];
	stream.cursor += string.length + 1;

	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			break;

		case ByteStreamMode::Read:
			string.data = data;
			break;

		case ByteStreamMode::Write:
			memcpy(data, string.data, string.length + 1);
			string.data = nullptr;
			break;
	}
}

void
Serialize(ByteStream& stream, StringView& string)
{
	c8* data = (c8*) &stream.bytes[stream.cursor];
	stream.cursor += string.length + 1;

	switch (stream.mode)
	{
		default: Assert(false); break;

		case ByteStreamMode::Size:
			break;

		case ByteStreamMode::Read:
			string.data = data;
			break;

		case ByteStreamMode::Write:
			memcpy(data, string.data, string.length + 1);
			string.data = nullptr;
			break;
	}
}

void
Serialize(ByteStream& stream, Message::PluginsAdded& pluginsAdded)
{
	Serialize(stream, pluginsAdded.header);
	Serialize(stream, pluginsAdded.kind);
	Serialize(stream, pluginsAdded.refs);
	Serialize(stream, pluginsAdded.infos);
}

void
Serialize(ByteStream& stream, Message::PluginStatesChanged& statesChanged)
{
	Serialize(stream, statesChanged.header);
	Serialize(stream, statesChanged.kind);
	Serialize(stream, statesChanged.refs);
	Serialize(stream, statesChanged.loadStates);
}

void
Serialize(ByteStream& stream, Message::SetPluginLoadStates& pluginloadStates)
{
	Serialize(stream, pluginloadStates.header);
	Serialize(stream, pluginloadStates.kind);
	Serialize(stream, pluginloadStates.refs);
	Serialize(stream, pluginloadStates.loadStates);
}

void
Serialize(ByteStream& stream, PluginInfo& pluginInfo)
{
	Serialize(stream, pluginInfo.name);
	Serialize(stream, pluginInfo.author);
	Serialize(stream, pluginInfo.version);
}

void
Serialize(ByteStream& stream, Message::SensorsAdded& sensorsAdded)
{
	Serialize(stream, sensorsAdded.header);
	Serialize(stream, sensorsAdded.pluginRefs);
	Serialize(stream, sensorsAdded.sensors);
}

void
Serialize(ByteStream& stream, Sensor& sensor)
{
	Serialize(stream, sensor.ref);
	Serialize(stream, sensor.name);
	Serialize(stream, sensor.identifier);
	Serialize(stream, sensor.format);
	Serialize(stream, sensor.value);
}

void
Serialize(ByteStream& stream, Message::WidgetDescsAdded& widgetDescsAdded)
{
	Serialize(stream, widgetDescsAdded.header);
	Serialize(stream, widgetDescsAdded.pluginRefs);
	Serialize(stream, widgetDescsAdded.descs);
}

void
Serialize(ByteStream& stream, WidgetDesc& widgetDesc)
{
	Serialize(stream, widgetDesc.ref);
	Serialize(stream, widgetDesc.name);
	Serialize(stream, widgetDesc.userDataSize);
	if (stream.mode == ByteStreamMode::Write)
	{
		widgetDesc.Initialize = nullptr;
		widgetDesc.Update     = nullptr;
		widgetDesc.Teardown   = nullptr;
	}
}

// TODO: Code gen the actual Serialize functions
// TODO: Maybe we can catch missed fields by counting bytes from serialize calls?
