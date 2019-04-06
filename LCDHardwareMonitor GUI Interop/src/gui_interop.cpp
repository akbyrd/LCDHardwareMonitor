// TODO: Spend up to 8 ms receiving messages
// TODO: What happens when the simulation tries to send a message larger than the pipe buffer size?
// TODO: Add loops to connecting, reading, and writing (predicated on making progress each iteration)
// TODO: Be careful to ensure the gui and sim never end up doing a blocking write simultaneously
// TODO: Decide whether to support multiple instances or enforce a single instance

#pragma unmanaged
#include "LHMAPI.h"

#include "platform.h"
#include "gui_protocol.h"

// Fuck you, Microsoft
#pragma warning(push, 0)
#pragma push_macro("IGNORE")
#undef IGNORE
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma pop_macro("IGNORE")
#pragma warning(pop)

#include "platform_win32.hpp"

struct State
{
	Pipe pipe;
	u32  activeMessageId;
};
static State s = {};

b32
Initialize()
{
	using namespace Message;

	PipeResult result = Platform_CreatePipeClient("LCDHardwareMonitor GUI Pipe", &s.pipe);
	LOG_IF(result == PipeResult::UnexpectedFailure, return false,
		Severity::Error, "Failed to create pipe for GUI communication");

	s.activeMessageId = Connect::Id;
	return true;
}

void
Teardown()
{
	// TODO: Is there anything sane we can do with failures?
	Platform_DisconnectPipe(&s.pipe);

	s = {};
}

#pragma managed
using namespace System::Collections::Generic;

// TODO: Implement these in pure C#. Don't waste time dicking with this bastard of a language
// TODO: Is there any way to use PluginHeader directly?
public value struct Source
{
	property System::String^ name;
	property System::String^ author;
	property System::UInt32  version;
};

// TODO: Can this be implemented as an extension method to System::String?
System::String^
ToSystemString(StringSlice cstring)
{
	LOG_IF((i32) cstring.length < 0, IGNORE,
		Severity::Warning, "Native string truncated");

	System::String^ result = gcnew System::String(cstring.data, 0, (i32) cstring.length);
	return result;
}

public value struct GUIInterop abstract sealed
{
	static b32
	Initialize()
	{
		return ::Initialize();
	}

	static b32
	Update(IList<Source>^ guiSources)
	{
		using namespace Message;

		Bytes bytes = {};
		defer { List_Free(bytes); };

		PipeResult result = ReceiveMessage(&s.pipe, bytes, &s.activeMessageId);
		switch (result)
		{
			default: Assert(false); break;

			case PipeResult::Success:
			{
				Header* header = (Header*) bytes.data;
				switch (header->id)
				{
					default: Assert(false); break;
					case Null::Id: break;

					case Connect::Id:
					{
						Connect* connect = (Connect*) bytes.data;
						Platform_Print("Received Connect! Version: %u\n", connect->version);
						break;
					}

					case Sources::Id:
					{
						ByteStream stream = {};
						stream.mode  = ByteStreamMode::Read;
						stream.bytes = bytes;

						Sources sources = {};
						b32 success = Sources_Serialize(sources, stream);
						LOG_IF(!success, return false,
							Severity::Error, "Failed to deserialize GUI message");

						Platform_Print("Received Sources!\n");
						for (u32 i = 0; i < sources.sources.length; i++)
						{
							PluginInfo info = sources.sources[i];
							Platform_Print("\t%s (v%u)\n", info.name.data, info.version);

							// TODO: Strip null terminator (at StringSlice creation)
							Source source = {};
							source.name    = ToSystemString(info.name);
							source.author  = ToSystemString(info.author);
							source.version = info.version;
							guiSources->Add(source);
						}
						break;
					}
				}
				break;
			}

			case PipeResult::TransientFailure:
				// Ignore failures, retry next frame
				break;

			case PipeResult::UnexpectedFailure:
				// TODO: Reset pipe connection?
				LOG(Severity::Error, "Failed to receive GUI message");
				return false;
		}

		return true;
	}

	static void
	Teardown()
	{
		return ::Teardown();
	}
};
