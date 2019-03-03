// TODO: Spend up to 8 ms receiving messages
// TODO: Set write timeout in simulation to avoid slowing it down (just skip writing until there's room)
// TODO: What happens when the simulation tries to send a message larger than the pipe buffer size?
// TODO: Fix warnings about _TP_POOL and _TP_CLEANUP_GROUP being used in in CLR meta-data (disable all meta-data?)
// TODO: Why is the output sometimes corrupted? Is it a problem with VS or our code?
// TODO: Add loops to connecting, reading, and writing (predicated on making progress each iteration)
// TODO: Does the server actually need to keep track of whether it's connected?

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
	b32 success = Platform_CreatePipeClient("LCDHardwareMonitor GUI Pipe", &s.pipe);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to create pipe for GUI communication");

	return true;
}

b32
Update()
{
	List<u8> bytes = {};
	b32 success = Platform_ReadPipe(&s.pipe, bytes);
	if (!success) return false;

	if (bytes.length > 0)
	{
		Platform_Print("Read %u bytes (", bytes.length);
		for (u32 i = 0; i < bytes.length; i++)
			Platform_Print("%hhu, ", bytes[i]);
		Platform_Print(") [%u]\n", GetCurrentThreadId());
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
