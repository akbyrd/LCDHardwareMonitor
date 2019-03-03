// TODO: Spend up to 8 ms receiving messages
// TODO: Set write timeout in simulation to avoid slowing it down
// TODO: What happens when the simulation tries to send a message larger than the pipe buffer size?
// TODO: Maybe just use native platform API for pipes?
// TODO: Fix warnings about _TP_POOL and _TP_CLEANUP_GROUP being used in in CLR meta-data (disable all meta-data?)

#pragma unmanaged
#include "LHMAPI.h"

#include "gui_protocol.h"
#include "platform.h"

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
	Pipe               pipe;
	GUIConnectionState connectionState;
	GUIMessage         activeMessage;
};
static State s = {};

b32
Initialize()
{
	b32 success = Platform_CreatePipeClient("LCDHardwareMonitor GUI Pipe", &s.pipe);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to create pipe for GUI communication");

	s.connectionState = GUIConnectionState::Connected;

	return true;
}

// TODO: We don't really need to manually reconnect contantly. It's built into read and write operations
b32
Update()
{
	switch (s.connectionState)
	{
		case GUIConnectionState::Connected:
		{
			List<u8> bytes = {};
			b32 success = Platform_ReadPipe(&s.pipe, bytes);
			if (!success) break;

			if (bytes.length > 0)
			{
				Platform_Print("Read %u bytes (", bytes.length);
				for (u32 i = 0; i < bytes.length; i++)
					Platform_Print("%hhu, ", bytes[i]);
				Platform_Print(")\n");
			}
			break;
		}

		case GUIConnectionState::Disconnected:
			// Ignore failures, retry next frame
			//Platform_ConnectPipe(&s.pipe);
			if (s.pipe.isConnected)
			{
				Platform_Print("Connected!\n");
				s.connectionState = GUIConnectionState::Connected;
			}
			break;

		default:
			Assert(false);
			break;
	}

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
//using namespace System;
//using namespace System::IO;
//using namespace System::IO::Pipes;

public value struct GUIInterop abstract sealed
{
#if false
	static void Tick(NamedPipeClientStream^ pipe, IAsyncResult^% pendingRead)
	{
		if (!pipe->IsConnected) return;

		array<Byte>^ bytes = gcnew array<Byte>(128);
		if (pendingRead == nullptr)
		{
			pipe->ReadMode = PipeTransmissionMode::Message;
			pendingRead = pipe->BeginRead(bytes, 0, bytes->Length, nullptr, nullptr);
		}
		else if (pendingRead->IsCompleted)
		{
			int read = pipe->EndRead(pendingRead);
			if (read > 0)
			{
				Console::Write("Read {0} bytes", read);
				Console::Write(" ({0}", bytes[0]);
				for (int i = 1; i < read; i++)
					Console::Write(", {0}", bytes[i]);
				Console::Write(")");
			}
			while (!pipe->IsMessageComplete)
			{
				int read2 = pipe->Read(bytes, 0, bytes->Length);
				Console::Write("  |  {0} more bytes", read2);
				Console::Write(" ({0}", bytes[0]);
				for (int i = 1; i < read; i++)
					Console::Write(", {0}", bytes[i]);
				Console::Write(")");
			}
			Console::WriteLine();

			//Handshake handshake = Foo.BytesToHandshake(bytes);

			pendingRead = nullptr;
		}
	}
#endif

	static b32
	Initialize()
	{
		return ::Initialize();
	}

	static b32
		Update()
	{
		return ::Update();
	}

	static void
	Teardown()
	{
		return ::Teardown();
	}
};
