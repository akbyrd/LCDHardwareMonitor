// TODO: Compare to .NET reference source
// TODO: Copy output to run folder
// TODO: Ensure compiler settings are uniform

// TODO: Spend up to 8 ms receiving messages
// TODO: What happens when the simulation tries to send a message larger than the pipe buffer size?
// TODO: Add loops to connecting, reading, and writing (predicated on making progress each iteration)
// TODO: Be careful to ensure the gui and sim never end up doing a blocking write simultaneously
// TODO: Decide whether to support multiple instances or enforce a single instance

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
	Pipe pipe;
};
static State s = {};

b32
Initialize()
{
	PipeResult result = Platform_CreatePipeClient("LCDHardwareMonitor GUI Pipe", &s.pipe);
	LOG_IF(result == PipeResult::UnexpectedFailure, return false,
		Severity::Error, "Failed to create pipe for GUI communication");

	return true;
}

b32
Update()
{
	List<u8> bytes = {};
	PipeResult result = Platform_ReadPipe(&s.pipe, bytes);
	if (result == PipeResult::UnexpectedFailure) return false;

	if (bytes.length > 0)
	{
		Platform_Print("Read %u bytes (", bytes.length);
		for (u32 i = 0; i < bytes.length; i++)
			Platform_Print("%hhu, ", bytes[i]);
		Platform_Print(")\n");
	}

	List_Free(bytes);
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
public value struct GUIInterop abstract sealed
{
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
