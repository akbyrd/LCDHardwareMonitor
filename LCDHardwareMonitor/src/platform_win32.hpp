// Fuck you, Microsoft
#pragma warning(push, 0)
#pragma push_macro("IGNORE")
#undef IGNORE
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#pragma pop_macro("IGNORE")
#pragma warning(pop)

template<u32 PlaceholderCount, typename... Args>
inline void
Platform_PrintChecked(StringView format, Args... args)
{
	static_assert(PlaceholderCount == sizeof...(Args));
	Platform_PrintImpl(format, args...);
}

void
Platform_PrintImpl(StringView message)
{
	// NOTE: printf/stdout do not appear in the Visual Studio Output window :(
	printf(message.data);
	if (IsDebuggerPresent())
		OutputDebugStringA(message.data);
}

template<typename... Args>
inline void
Platform_PrintImpl(StringView format, Args... args)
{
	String message = String_FormatImpl(format, args...);
	defer { List_Free(message); };

	Platform_PrintImpl(message);
}

template<u32 PlaceholderCount, typename... Args>
inline void
Platform_LogChecked(Severity severity, Location location, StringView format, Args... args)
{
	static_assert(PlaceholderCount == sizeof...(Args));
	Platform_LogImpl(severity, location, format, args...);
}

void
Platform_LogImpl(Severity severity, Location location, StringView message)
{
	Assert(severity != Severity::Null);

	String fullMessage = String_FormatImpl("% - %\n\t%(%)\n", location.function, message, location.file, location.line);

	Platform_PrintImpl(fullMessage);
	if (severity > Severity::Info)
		__debugbreak();
}

template<typename... Args>
inline void
Platform_LogImpl(Severity severity, Location location, StringView format, Args... args)
{
	String message = String_FormatImpl(format, args...);
	defer { String_Free(message); };

	Platform_LogImpl(severity, location, message);
}

void
LogFormatMessage(u32 messageID, Severity severity, Location location, StringView message)
{
	c8* windowsMessage = 0;
	defer { LocalFree(windowsMessage); };
	u32 uResult = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		nullptr,
		messageID,
		0,
		(c8*) &windowsMessage,
		0,
		nullptr
	);
	if (uResult == 0)
	{
		// TODO: Try to print the last error once
		Platform_PrintImpl("FormatMessageA failed. Remaining message:\n\t");
		Platform_LogImpl(severity, location, message);
		return;
	}

	// Strip trailing new line
	u32 length = uResult;
	while (length > 0 && (windowsMessage[length - 1] == '\n' || windowsMessage[length - 1] == '\r'))
		windowsMessage[(length--) - 1] = '\0';

	Platform_LogImpl(severity, location, "%: % (%)", message, windowsMessage, messageID);
}

template<typename... Args>
inline void
LogFormatMessage(u32 messageID, Severity severity, Location location, StringView format, Args... args)
{
	String message = String_FormatImpl(format, args...);
	defer { List_Free(message); };

	LogFormatMessage(messageID, severity, location, message);
}

void
LogHRESULT(HRESULT hr, Severity severity, Location location, StringView message)
{
	// TODO: This cast is weird. I feel like either LogHRESULT or LogLastError is doing the wrong
	// thing. There's a function for converting an error to an HRESULT which implies the difference
	// is meaningful.
	// NOTE: TODO? CLR HRESULTS aren't found by FormatMessage. Start with 0x8013. Defined in
	// corerror.h/mscorrc.dll
	LogFormatMessage((u32) hr, severity, location, message);
}

template<typename... Args>
inline void
LogHRESULT(HRESULT hr, Severity severity, Location location, StringView format, Args... args)
{
	String message = String_FormatImpl(format, args...);
	defer { String_Free(message); };

	LogHRESULT(hr, severity, location, message);
}
#define LOG_HRESULT(hr, severity, format, ...) LogHRESULT(hr, severity, LOCATION, format, ##__VA_ARGS__)
#define LOG_HRESULT_IF_FAILED(hr, action, severity, format, ...) IF(FAILED(hr), LOG_HRESULT(hr, severity, format, ##__VA_ARGS__); action)

void
LogLastError(Severity severity, Location location, StringView message)
{
	u32 lastError = GetLastError();
	LogFormatMessage(lastError, severity, location, message);
}

template<typename... Args>
inline void
LogLastError(Severity severity, Location location, StringView format, Args... args)
{
	String message = String_FormatImpl(format, args...);
	defer { String_Free(message); };

	LogLastError(severity, location, message);
}
#define LOG_LAST_ERROR(severity, format, ...) LogLastError(severity, LOCATION, format, ##__VA_ARGS__)
#define LOG_LAST_ERROR_IF(expression, action, severity, format, ...) IF(expression, LOG_LAST_ERROR(severity, format, ##__VA_ARGS__); action)

// TODO: RAII wrapper
static String
GetWorkingDirectory()
{
	String result = {};

	u32 length = GetCurrentDirectoryA(0, nullptr);
	LOG_LAST_ERROR_IF(!length, return result,
		Severity::Warning, "Failed to get working directory length");

	String_Reserve(result, length);

	u32 written = GetCurrentDirectoryA(length, result.data);
	LOG_LAST_ERROR_IF(!written, return result,
		Severity::Warning, "Failed to get working directory");

	// NOTE: length includes null terminator, written does not
	LOG_IF((written + 1) != length, return result,
		Severity::Warning, "Working directory length does not match expected");

	result.length = length;
	return result;
}

b8
Platform_WriteFileBytes(StringView path, ByteSlice bytes)
{
	String cwd = {};
	defer { String_Free(cwd); };
	auto getCWD = [&cwd]() {
		cwd = GetWorkingDirectory();
		return cwd;
	};

	HANDLE file = CreateFileA(
		path.data,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		nullptr
	);
	defer { CloseHandle(file); };
	LOG_LAST_ERROR_IF(file == INVALID_HANDLE_VALUE, return false,
		Severity::Warning, "Failed to create file handle '%'; CWD: '%'", path, getCWD());

	DWORD bytesWritten;
	b8 success = WriteFile(file, bytes.data, bytes.length, &bytesWritten, nullptr);
	LOG_LAST_ERROR_IF(!success, return false,
		Severity::Warning, "Failed to write file '%'; CWD: '%'", path, getCWD());
	LOG_LAST_ERROR_IF(bytesWritten != bytes.length, return false,
		Severity::Warning, "Failed to write entire file '%'; CWD: '%'", path, getCWD());

	return true;
}

static Bytes
LoadFile(StringView path, u32 padding = 0)
{
	Bytes result = {};
	auto resultGuard = guard { List_Free(result); };

	String cwd = {};
	defer { String_Free(cwd); };
	auto getCWD = [&cwd]() {
		cwd = GetWorkingDirectory();
		return cwd;
	};

	{
		HANDLE file = CreateFileA(
			path.data,
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			nullptr
		);
		defer { CloseHandle(file); };
		LOG_LAST_ERROR_IF(file == INVALID_HANDLE_VALUE, return result,
			Severity::Warning, "Failed to create file handle '%'; CWD: '%'", path, getCWD());

		LARGE_INTEGER size_win32;
		b8 success = GetFileSizeEx(file, &size_win32);
		LOG_LAST_ERROR_IF(!success, return result,
			Severity::Warning, "Failed to get file size '%'", path);
		LOG_IF(size_win32.QuadPart > u32Max - padding, return result,
			Severity::Warning, "File is too large to load '%'", path);

		u32 size = (u32) size_win32.QuadPart;
		List_Reserve(result, size + padding);

		success = ReadFile(file, result.data, size, (DWORD*) &result.length, nullptr);
		LOG_LAST_ERROR_IF(!success, return result,
			Severity::Warning, "Failed to read file '%'", path);
	}

	resultGuard.dismiss = true;
	return result;
}

Bytes
Platform_LoadFileBytes(StringView path)
{
	return LoadFile(path, 0);
}

String
Platform_LoadFileString(StringView path)
{
	String result = {};

	Bytes bytes = LoadFile(path, 1);
	if (!bytes.data) return result;

	result.length   = bytes.length;
	result.capacity = bytes.capacity;
	result.data     = (c8*) bytes.data;

	result[result.length++] = '\0';

	return result;
}

i64
Platform_GetTicks()
{
	// NOTE: Never fails above XP
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return (i64) counter.QuadPart;
}

r32
Platform_TicksToSeconds(i64 ticks)
{
	// NOTE: Never fails above XP
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return (r32) ticks / (r32) frequency.QuadPart;
}

r32
Platform_GetElapsedSeconds(i64 startTicks)
{
	i64 elapsedTicks = Platform_GetTicks() - startTicks;
	return Platform_TicksToSeconds(elapsedTicks);
}

r32
Platform_GetElapsedMilliseconds(i64 startTicks)
{
	i64 elapsedTicks = Platform_GetTicks() - startTicks;
	return Platform_TicksToSeconds(elapsedTicks) * 1000.0f;
}

void
Platform_RequestQuit()
{
	PostQuitMessage(0);
}

struct PipeImpl
{
	String     fullName;
	HANDLE     handle;
	OVERLAPPED connect;
};

inline b8
IsValidHandle(HANDLE handle)
{
	// NOTE: Some Win32 functions return nullptr and some return INVALID_HANDLE_VALUE on failure.
	// Things would be fine if that were the end of it. But some functions take 'pseudohandles' and
	// INVALID_HANDLE_VALUE can be a valid pseudohandle. Fuck you, Microsoft.
	return handle != nullptr && handle != INVALID_HANDLE_VALUE;
}

PipeResult
Platform_DisconnectPipeServer(Pipe& pipe)
{
	PipeState state = pipe.state;
	pipe.state = PipeState::Disconnecting;

	switch (state)
	{
		case PipeState::Null: Assert(false); break;

		case PipeState::Connecting:
		{
			b8 success = CancelIoEx(
				pipe.impl->handle,
				&pipe.impl->connect
			);
			LOG_LAST_ERROR_IF(!success, return PipeResult::UnexpectedFailure,
				Severity::Warning, "Failed to cancel pending pipe server connection '%'", pipe.name);
			break;
		}

		case PipeState::Disconnected:
			break;

		case PipeState::Connected:
		case PipeState::Disconnecting:
		{
			b8 success = DisconnectNamedPipe(pipe.impl->handle);
			LOG_LAST_ERROR_IF(!success, return PipeResult::UnexpectedFailure,
				Severity::Warning, "Failed to disconnect pipe server '%'", pipe.name);
			break;
		}
	}

	pipe.state = PipeState::Disconnected;
	return PipeResult::Success;
}

PipeResult
Platform_DisconnectPipeClient(Pipe& pipe)
{
	PipeState state = pipe.state;
	pipe.state = PipeState::Disconnecting;

	switch (state)
	{
		case PipeState::Null: Assert(false); break;

		// Not used for client pipes
		case PipeState::Connecting:
			Assert(false);
			break;

		case PipeState::Disconnected:
			break;

		case PipeState::Connected:
		case PipeState::Disconnecting:
		{
			b8 success = CloseHandle(pipe.impl->handle);
			LOG_LAST_ERROR_IF(!success, return PipeResult::UnexpectedFailure,
				Severity::Warning, "Failed to disconnect pipe client '%'", pipe.name);

			pipe.impl->handle = nullptr;
			break;
		}
	}

	pipe.state = PipeState::Disconnected;
	return PipeResult::Success;
}

PipeResult
Platform_DisconnectPipe(Pipe& pipe)
{
	return pipe.isServer
		? Platform_DisconnectPipeServer(pipe)
		: Platform_DisconnectPipeClient(pipe);
}

PipeResult
Platform_ConnectPipeServer(Pipe& pipe)
{
	// Create the platform pipe
	if (!IsValidHandle(pipe.impl->handle))
	{
		// NOTE: *MUST* specify FILE_FLAG_OVERLAPPED or ConnectNamedPipe stalls even when providing
		// an overlapped struct.
		pipe.impl->handle = CreateNamedPipeA(
			pipe.impl->fullName.data,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
			1,
			// TODO: See remarks
			1024, 1024,
			0,
			nullptr
		);
		if (pipe.impl->handle == INVALID_HANDLE_VALUE)
		{
			u32 error = GetLastError();
			switch (error)
			{
				// Another server has created the pipe
				case ERROR_PIPE_BUSY:
					return PipeResult::TransientFailure;

				default:
					LOG_LAST_ERROR(Severity::Warning, "Failed to create pipe server '%'", pipe.name);
					return PipeResult::UnexpectedFailure;
			}
		}
	}

	// NOTE: Though it's not documented clearly, it appears you *must* call ConnectNamedPipe for a
	// client to be able to connect. It's not an optional way to wait/check for clients. A client
	// will be able to connect once, but after it disconnects (and even after the server calls
	// DiconnectNamedPipe) it won't be able to reconnect again.

	switch (pipe.state)
	{
		case PipeState::Null: Assert(false); break;

		case PipeState::Connected:
			return PipeResult::Success;

		case PipeState::Disconnecting:
			return PipeResult::TransientFailure;

		// Begin a connection
		case PipeState::Disconnected:
		{
			b8 success = ConnectNamedPipe(pipe.impl->handle, &pipe.impl->connect);
			if (!success)
			{
				u32 error = GetLastError();
				switch (error)
				{
					// Connection is pending
					case ERROR_IO_PENDING:
						pipe.state = PipeState::Connecting;
						return PipeResult::TransientFailure;

					// Already connected
					case ERROR_PIPE_CONNECTED:
						break;

					// Pipe is being closed
					case ERROR_NO_DATA:
					{
						PipeResult result = Platform_DisconnectPipe(pipe);
						if (result != PipeResult::Success) return result;
						return PipeResult::TransientFailure;
					}

					default:
						LOG_LAST_ERROR(Severity::Warning, "Failed to begin pipe connection");
						return PipeResult::UnexpectedFailure;
				}
			}
			break;
		}

		// Wait for a pending connection
		case PipeState::Connecting:
		{
			u32 written;
			b8 success = GetOverlappedResult(
				pipe.impl->handle,
				&pipe.impl->connect,
				(DWORD*) &written,
				false
			);
			if (!success)
			{
				u32 error = GetLastError();
				switch (error)
				{
					// Connection is still pending
					case ERROR_IO_INCOMPLETE:
						return PipeResult::TransientFailure;

					default:
						LOG_LAST_ERROR(Severity::Error, "Pipe connection check failed");
						return PipeResult::UnexpectedFailure;
				}
			}
			break;
		}
	}

	pipe.state = PipeState::Connected;
	return PipeResult::Success;
}

PipeResult
Platform_ConnectPipeClient(Pipe& pipe)
{
	if (IsValidHandle(pipe.impl->handle)) return PipeResult::Success;

	auto cleanupGuard = guard {
		CloseHandle(pipe.impl->handle);
		pipe.impl->handle = nullptr;
	};

	switch (pipe.state)
	{
		case PipeState::Null: Assert(false); break;

		// Impossible for client pipes
		case PipeState::Connecting:
			Assert(false);
			break;

		case PipeState::Connected:
			break;

		case PipeState::Disconnecting:
			return PipeResult::TransientFailure;

		case PipeState::Disconnected:
		{
			pipe.impl->handle = CreateFileA(
				pipe.impl->fullName.data,
				GENERIC_READ | GENERIC_WRITE,
				0,
				nullptr,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				nullptr
			);
			if (pipe.impl->handle == INVALID_HANDLE_VALUE)
			{
				u32 error = GetLastError();
				switch (error)
				{
					// Server hasn't created the pipe yet
					case ERROR_FILE_NOT_FOUND:
						return PipeResult::TransientFailure;

					// Max clients already connected
					case ERROR_PIPE_BUSY:
						return PipeResult::TransientFailure;

					default:
						LOG_LAST_ERROR(Severity::Error, "Failed to create pipe client '%'", pipe.name);
						return PipeResult::UnexpectedFailure;
				}
			}

			u32 mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
			b8 success = SetNamedPipeHandleState(
				pipe.impl->handle,
				(DWORD*) &mode,
				nullptr,
				nullptr
			);
			LOG_LAST_ERROR_IF(!success, return PipeResult::UnexpectedFailure,
				Severity::Warning, "Failed to set pipe client mode '%'", pipe.name);
			break;
		}
	}

	cleanupGuard.dismiss = true;
	pipe.state = PipeState::Connected;
	return PipeResult::Success;
}

PipeResult
Platform_ConnectPipe(Pipe& pipe)
{
	return pipe.isServer
		? Platform_ConnectPipeServer(pipe)
		: Platform_ConnectPipeClient(pipe);
}

PipeResult
Platform_UpdatePipeConnection(Pipe& pipe)
{
	PipeResult result = Platform_ConnectPipe(pipe);
	if (result != PipeResult::Success) return result;

	u32 available = 0;
	b8 success = PeekNamedPipe(
		pipe.impl->handle,
		nullptr,
		0,
		nullptr,
		nullptr,
		(DWORD*) &available
	);
	if (!success)
	{
		u32 error = GetLastError();
		switch (error)
		{
			// Pipe has been closed
			case ERROR_BROKEN_PIPE:
			case ERROR_PIPE_NOT_CONNECTED:
			case ERROR_NO_DATA:
				Assert(available == 0);
				result = Platform_DisconnectPipe(pipe);
				return result;

			default:
				LOG_LAST_ERROR(Severity::Warning, "Failed to update pipe connection '%'", pipe.name);
				return PipeResult::UnexpectedFailure;
		}
	}

	return PipeResult::Success;
}

PipeResult
Platform_CreatePipeServer(StringView name, Pipe& pipe)
{
	auto cleanupGuard = guard { Platform_DestroyPipe(pipe); };

	// Initialize
	{
		pipe = {};
		pipe.name     = String_FromView(name);
		pipe.state    = PipeState::Disconnected;
		pipe.isServer = true;
	}

	// Allocate platform data
	{
		pipe.impl = (PipeImpl*) AllocChecked(sizeof(PipeImpl));
		*pipe.impl = {};

		pipe.impl->fullName = String_Format("\\\\.\\pipe\\%", pipe.name);
	}

	// Create connection event
	{
		// TODO: This is crazy town. Find confirmation for this behavior.
		// NOTE: Windows holds the pointer to the overlapped struct and will update the Internal
		// status asynchronously.

		pipe.impl->connect.hEvent = CreateEventA(nullptr, true, true, nullptr);
		LOG_LAST_ERROR_IF(pipe.impl->connect.hEvent == INVALID_HANDLE_VALUE, return PipeResult::UnexpectedFailure,
			Severity::Warning, "Failed to create pipe server connect event '%'", pipe.name);
	}

	// TODO: Would like to automatically kick of a connection here, but that makes it harder for the
	// caller to handle connect/disconnect events.
	cleanupGuard.dismiss = true;
	return PipeResult::Success;
}

PipeResult
Platform_CreatePipeClient(StringView name, Pipe& pipe)
{
	auto cleanupGuard = guard { Platform_DestroyPipe(pipe); };

	// Initialize
	{
		pipe = {};
		pipe.name     = String_FromView(name);
		pipe.state    = PipeState::Disconnected;
		pipe.isServer = false;
	}

	// Allocate platform data
	{
		pipe.impl = (PipeImpl*) AllocChecked(sizeof(PipeImpl));
		*pipe.impl = {};

		pipe.impl->fullName = String_Format("\\\\.\\pipe\\%", pipe.name);
	}

	// Attempt to connect
	{
		cleanupGuard.dismiss = true;

		PipeResult result = Platform_ConnectPipeClient(pipe);
		if (result != PipeResult::Success) return result;
	}

	return PipeResult::Success;
}

void
Platform_DestroyPipe(Pipe& pipe)
{
	String_Free(pipe.name);
	String_Free(pipe.impl->fullName);
	// TODO: Handle failure?
	CloseHandle(pipe.impl->handle);
	CloseHandle(pipe.impl->connect.hEvent);
	Free(pipe.impl);
	pipe = {};
}

// TODO: This should be able to take a slice
PipeResult
Platform_WritePipe(Pipe& pipe, Bytes bytes)
{
	// NOTE: Writes are synchronous. If this changes be aware of the following facts: 1) The
	// 'written' parameter is not valid for async writes and 2) the data buffer MUST NOT CHANGE
	// during an async write (i.e. build a buffer of outgoing data).

	// NOTE: Synchronous writes will begin blocking once the pipes internal buffer is full.

	if (pipe.state != PipeState::Connected) return PipeResult::TransientFailure;

	u32 written;
	b8 success = WriteFile(
		pipe.impl->handle,
		bytes.data,
		bytes.length,
		(DWORD*) &written,
		nullptr
	);
	if (!success)
	{
		u32 error = GetLastError();
		switch (error)
		{
			// The pipe is being closed
			case ERROR_NO_DATA:
				// TODO: Would like to set Disconnecting state here, but that makes it harder for the
				// caller to handle connect/disconnect events.
				return PipeResult::TransientFailure;

			default:
				LOG_LAST_ERROR(Severity::Warning, "Writing to pipe failed '%'", pipe.name);
				return PipeResult::UnexpectedFailure;
		}
	}

	// NOTE: This is a severe error, since it leaves the reader in an unpredictably corrupted state.
	// However, we control both the reader and the writer and can account for this at a higher level.
	// The simulation includes the size in a fixed size header at the beginning of each message. If
	// the reader gets a message smaller than the header size it can ignore it. If the reader gets a
	// message that is bigger than the header but doesn't match the expected size it can ignore it.
	// The sender will get a failure message and simply resend the message next time it gets a
	// chance.
	LOG_IF(written != bytes.length, return PipeResult::TransientFailure,
		Severity::Fatal, "Writing to pipe truncated '%'", pipe.name);

	return PipeResult::Success;
}

PipeResult
Platform_ReadPipe(Pipe& pipe, Bytes& bytes)
{
	bytes.length = 0;

	if (pipe.state != PipeState::Connected) return PipeResult::TransientFailure;

	u32 available = 0;
	b8 success = PeekNamedPipe(
		pipe.impl->handle,
		nullptr,
		0,
		nullptr,
		nullptr,
		(DWORD*) &available
	);
	if (!success)
	{
		u32 error = GetLastError();
		switch (error)
		{
			// TODO: What is the exact cause of each of these errors?
			// Pipe has been closed
			case ERROR_BROKEN_PIPE:
			case ERROR_PIPE_NOT_CONNECTED:
			case ERROR_NO_DATA:
				Assert(available == 0);
				// TODO: Would like to set Disconnecting state here, but that makes it harder for the
				// caller to handle connect/disconnect events.
				return PipeResult::TransientFailure;

			default:
				LOG_LAST_ERROR(Severity::Warning, "Failed to peek pipe '%'", pipe.name);
				return PipeResult::UnexpectedFailure;
		}
	}

	if (available == 0) return PipeResult::Success;
	List_Reserve(bytes, available);

	u32 read;
	success = ReadFile(
		pipe.impl->handle,
		bytes.data,
		available,
		(DWORD*) &read,
		nullptr
	);
	if (!success)
	{
		u32 error = GetLastError();
		switch (error)
		{
			// Read is pending
			case ERROR_IO_PENDING:
				return PipeResult::TransientFailure;

			default:
				LOG_LAST_ERROR(Severity::Warning, "Reading from pipe failed '%'", pipe.name);
				return PipeResult::UnexpectedFailure;
		}
	}

	// TODO: Could potentially drop the connection instead of fataling. Or implement an ack for each
	// message
	LOG_IF(read != available, return PipeResult::UnexpectedFailure,
		Severity::Fatal, "Read wrong amount of data from pipe '%'", pipe.name);

	bytes.length = read;
	return PipeResult::Success;
}

PipeResult
Platform_FlushPipe(Pipe& pipe)
{
	if (!IsValidHandle(pipe.impl->handle)) return PipeResult::TransientFailure;

	b8 success = FlushFileBuffers(pipe.impl->handle);
	LOG_LAST_ERROR_IF(!success, return PipeResult::UnexpectedFailure,
		Severity::Error, "Failed to flush pipe '%'", pipe.name);

	return PipeResult::Success;
}
