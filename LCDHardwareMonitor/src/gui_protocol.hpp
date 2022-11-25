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
}

namespace ToGUI
{
	using namespace Message;

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
		Header                header;
		Slice<Handle<Plugin>> handles;
		Slice<PluginKind>     kinds;
		Slice<PluginInfo>     infos;
		Slice<PluginLanguage> languages;
	};

	struct PluginStatesChanged
	{
		Header                 header;
		Slice<Handle<Plugin>>  handles;
		Slice<PluginKind>      kinds;
		Slice<PluginLoadState> loadStates;
	};

	// TODO: SensorsRemoved
	struct SensorsAdded
	{
		Header        header;
		Slice<Sensor> sensors;
	};

	// TODO: WidgetDescsRemoved
	struct WidgetDescsAdded
	{
		Header                    header;
		Slice<Handle<WidgetDesc>> handles;
		Slice<String>             names;
	};

	// TODO: WidgetsRemoved
	struct WidgetsAdded
	{
		Header                header;
		Slice<Handle<Widget>> handles;
	};

	struct WidgetSelectionChanged
	{
		Header                header;
		Slice<Handle<Widget>> handles;
	};
}

namespace FromGUI
{
	using namespace Message;

	struct TerminateSimulation
	{
		Header header;
	};

	struct SetPluginLoadStates
	{
		Header                 header;
		Slice<Handle<Plugin>>  handles;
		Slice<PluginLoadState> loadStates;
	};

	struct MouseMove
	{
		Header header;
		v2i    pos;
	};

	struct SelectHovered
	{
		Header header;
	};

	struct BeginMouseLook
	{
		Header header;
	};

	struct EndMouseLook
	{
		Header header;
	};

	struct ResetCamera
	{
		Header header;
	};

	struct DragDrop
	{
		Header     header;
		PluginKind pluginKind;
		b8         inProgress;
	};

	struct AddWidget
	{
		Header             header;
		Handle<WidgetData> handle;
		v2                 position;
	};

	struct RemoveWidget
	{
		Header         header;
		Handle<Widget> handle;
	};

	struct RemoveSelectedWidgets
	{
		Header header;
	};

	struct BeginDragSelection
	{
		Header header;
	};

	struct EndDragSelection
	{
		Header header;
	};

	struct SetWidgetSelection
	{
		Header                header;
		Slice<Handle<Widget>> handles;
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
		List_Free(con.queue.data[i]);
	List_Free(con.queue);

	con = {};
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

void Serialize(ByteStream&, ToGUI::PluginsAdded&);
void Serialize(ByteStream&, ToGUI::PluginStatesChanged&);
void Serialize(ByteStream&, ToGUI::SensorsAdded&);
void Serialize(ByteStream&, ToGUI::WidgetDescsAdded&);
void Serialize(ByteStream&, ToGUI::WidgetsAdded&);
void Serialize(ByteStream&, ToGUI::WidgetSelectionChanged&);
void Serialize(ByteStream&, FromGUI::SetPluginLoadStates&);
void Serialize(ByteStream&, FromGUI::SetWidgetSelection&);
void Serialize(ByteStream&, PluginInfo&);
void Serialize(ByteStream&, Sensor&);

inline b8
MessageTimeLeft(i64 startTicks)
{
	// TODO: Why does this cause C4738?
	return Platform_GetElapsedMilliseconds(startTicks) < 8.f;
}

template<typename T>
void
SerializeMessage(Bytes& bytes, T& message, u32 messageIndex)
{
	ByteStream stream = {};
	defer { List_Free(stream.bytes); };

	stream.mode = ByteStreamMode::Size;
	Serialize(stream, &message);

	List_Reserve(stream.bytes, stream.cursor);
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
}

template<typename T>
void
DeserializeMessage(Bytes& bytes)
{
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
		case PipeResult::Null: Assert(false); break;
		case PipeResult::Success: break;
		case PipeResult::TransientFailure: break;

		default:
		case PipeResult::UnexpectedFailure:
			HandleMessageResult(con, false);
			break;
	}

	return result;
}

void
QueueMessage(ConnectionState& con, Bytes& bytes)
{
	List_Append(con.queue, bytes);
	con.sendIndex++;
}

// TODO: Can probably simplify this now that it's no-fail
template<typename T>
void
SerializeAndQueueMessage(ConnectionState& con, T& message)
{
	Bytes bytes = {};
	SerializeMessage(bytes, message, con.sendIndex);
	QueueMessage(con, bytes);
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

	Message::Header& header = (Message::Header&) bytes[0];

	LOG_IF(bytes.length < sizeof(Message::Header), return HandleMessageResult(con, false),
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
		default:
		case ByteStreamMode::Null:
			Assert(false);
			break;

		case ByteStreamMode::Size:
			break;

		case ByteStreamMode::Read:
		{
			// Point to the data appended to the stream
			T* data = (T*) &stream.bytes.data[stream.cursor];
			pointer = data;
			break;
		}

		case ByteStreamMode::Write:
		{
			// Copy the data into the stream
			T* data = (T*) &stream.bytes.data[stream.cursor];
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
	Unused(value, stream);
}

template<typename T>
void
Serialize(ByteStream& stream, List<T>& list)
{
	T* data = (T*) &stream.bytes.data[stream.cursor];
	stream.cursor += List_SizeOf(list);

	switch (stream.mode)
	{
		default:
		case ByteStreamMode::Null:
			Assert(false);
			break;

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
	T* data = (T*) &stream.bytes.data[stream.cursor];
	stream.cursor += Slice_SizeOf(slice);

	switch (stream.mode)
	{
		default:
		case ByteStreamMode::Null:
			Assert(false);
			break;

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

			slice.data   = nullptr;
			break;
		}
	}
}

void
Serialize(ByteStream& stream, String& string)
{
	c8* data = (c8*) &stream.bytes.data[stream.cursor];
	stream.cursor += string.length + 1;

	switch (stream.mode)
	{
		default:
		case ByteStreamMode::Null:
			Assert(false);
			break;

		case ByteStreamMode::Size:
			break;

		case ByteStreamMode::Read:
			string.data = data;
			break;

		case ByteStreamMode::Write:
			if (string.data == nullptr)
			{
				data[0] = 0;
			}
			else
			{
				memcpy(data, string.data, string.length + 1);
				string.data = nullptr;
			}
			break;
	}
}

void
Serialize(ByteStream& stream, StringView& string)
{
	c8* data = (c8*) &stream.bytes.data[stream.cursor];
	stream.cursor += string.length + 1;

	switch (stream.mode)
	{
		default:
		case ByteStreamMode::Null:
			Assert(false);
			break;

		case ByteStreamMode::Size:
			break;

		case ByteStreamMode::Read:
			string.data = data;
			break;

		case ByteStreamMode::Write:
			if (string.data == nullptr)
			{
				data[0] = 0;
			}
			else
			{
				memcpy(data, string.data, string.length + 1);
				string.data = nullptr;
			}
			break;
	}
}

void
Serialize(ByteStream& stream, ToGUI::PluginsAdded& pluginsAdded)
{
	Serialize(stream, pluginsAdded.header);
	Serialize(stream, pluginsAdded.handles);
	Serialize(stream, pluginsAdded.kinds);
	Serialize(stream, pluginsAdded.infos);
	Serialize(stream, pluginsAdded.languages);
}

void
Serialize(ByteStream& stream, ToGUI::PluginStatesChanged& statesChanged)
{
	Serialize(stream, statesChanged.header);
	Serialize(stream, statesChanged.handles);
	Serialize(stream, statesChanged.kinds);
	Serialize(stream, statesChanged.loadStates);
}

void
Serialize(ByteStream& stream, FromGUI::SetPluginLoadStates& pluginloadStates)
{
	Serialize(stream, pluginloadStates.header);
	Serialize(stream, pluginloadStates.handles);
	Serialize(stream, pluginloadStates.loadStates);
}

void
Serialize(ByteStream& stream, FromGUI::SetWidgetSelection& widgetSelection)
{
	Serialize(stream, widgetSelection.header);
	Serialize(stream, widgetSelection.handles);
}

void
Serialize(ByteStream& stream, PluginInfo& pluginInfo)
{
	Serialize(stream, pluginInfo.name);
	Serialize(stream, pluginInfo.author);
	Serialize(stream, pluginInfo.version);
}

void
Serialize(ByteStream& stream, ToGUI::SensorsAdded& sensorsAdded)
{
	Serialize(stream, sensorsAdded.header);
	Serialize(stream, sensorsAdded.sensors);
}

void
Serialize(ByteStream& stream, Sensor& sensor)
{
	Serialize(stream, sensor.handle);
	Serialize(stream, sensor.name);
	Serialize(stream, sensor.identifier);
	Serialize(stream, sensor.format);
	Serialize(stream, sensor.value);
}

void
Serialize(ByteStream& stream, ToGUI::WidgetDescsAdded& widgetDescsAdded)
{
	Serialize(stream, widgetDescsAdded.header);
	Serialize(stream, widgetDescsAdded.handles);
	Serialize(stream, widgetDescsAdded.names);
}

void
Serialize(ByteStream& stream, WidgetDesc& widgetDesc)
{
	Serialize(stream, widgetDesc.handle);
	Serialize(stream, widgetDesc.name);
	Serialize(stream, widgetDesc.userDataSize);
	if (stream.mode == ByteStreamMode::Write)
	{
		widgetDesc.Initialize = nullptr;
		widgetDesc.Update     = nullptr;
		widgetDesc.Teardown   = nullptr;
	}
}

void
Serialize(ByteStream& stream, ToGUI::WidgetsAdded& widgetsAdded)
{
	Serialize(stream, widgetsAdded.header);
	Serialize(stream, widgetsAdded.handles);
}

void
Serialize(ByteStream& stream, ToGUI::WidgetSelectionChanged& widgetSelection)
{
	Serialize(stream, widgetSelection.header);
	Serialize(stream, widgetSelection.handles);
}

// TODO: Code gen the actual Serialize functions
// TODO: Maybe we can catch missed fields by counting bytes from serialize calls?
