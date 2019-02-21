void
Platform_Print(c8* message)
{
	// NOTE: printf/stdout do not appear in the Visual Studio Output window :(
	printf(message);
	if (IsDebuggerPresent())
		OutputDebugStringA(message);
}

template<typename... Args>
inline void
Platform_Print(c8* format, Args... args)
{
	String message = {};
	defer { List_Free(message); };

	b32 success = String_Format(message, format, args...);
	if (!success)
	{
		Platform_Print(format);
		return;
	}

	Platform_Print(message.data);
}

void
Platform_Log(Severity severity, Location location, c8* message)
{
	Assert(severity != Severity::Null);

	String fullMessage = {};
	defer { List_Free(fullMessage); };

	b32 success = String_Format(fullMessage, "%s - %s\n\t%s(%i)\n", location.function, message, location.file, location.line);
	if (!success)
	{
		Platform_Print("String_Format failed. Unformatted message:\n\t");
		Platform_Print(location.function);
		Platform_Print(" - ");
		Platform_Print(message);
		Platform_Print("\n\t");
		Platform_Print(location.file);
		Platform_Print("\n");
	}

	Platform_Print(fullMessage.data);
	if (severity > Severity::Info)
		__debugbreak();
}

template<typename... Args>
inline void
Platform_Log(Severity severity, Location location, c8* format, Args... args)
{
	String message = {};
	defer { List_Free(message); };

	b32 success = String_Format(message, format, args...);
	if (!success)
	{
		Platform_Print("String_Format failed. Unformatted message:\n\t");
		Platform_Print(format);
		return;
	}

	Platform_Log(severity, location, message.data);
}

void
LogFormatMessage(u32 messageID, Severity severity, Location location, c8* message)
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
		Platform_Print("FormatMessageA failed. Remaining message:\n\t");
		Platform_Log(severity, location, message);
		return;
	}

	// Strip trailing new line
	u32 length = uResult;
	while (length > 0 && (windowsMessage[length - 1] == '\n' || windowsMessage[length - 1] == '\r'))
		windowsMessage[(length--) - 1] = '\0';

	Platform_Log(severity, location, "%s: %s (%u)", message, windowsMessage, messageID);
}

template<typename... Args>
inline void
LogFormatMessage(u32 messageID, Severity severity, Location location, c8* format, Args... args)
{
	String message = {};
	defer { List_Free(message); };

	b32 success = String_Format(message, format, args...);
	if (!success)
	{
		LogFormatMessage(messageID, severity, location, format);
		return;
	}

	LogFormatMessage(messageID, severity, location, message);
}

void
LogHRESULT(HRESULT hr, Severity severity, Location location, c8* message)
{
	// TODO: This cast is weird. I feel like either LogHRESULT or LogLastError is doing the wrong
	// thing. There's a function for converting an error to an HRESULT which implies the difference
	// is meaningful.
	LogFormatMessage((u32) hr, severity, location, message);
}

template<typename... Args>
inline void
LogHRESULT(HRESULT hr, Severity severity, Location location, c8* format, Args... args)
{
	String message = {};
	defer { List_Free(message); };

	b32 success = String_Format(message, format, args...);
	if (!success)
	{
		LogHRESULT(hr, severity, location, format);
		return;
	}

	LogHRESULT(hr, severity, location, message.data);
}
#define LOG_HRESULT(hr, severity, format, ...) LogHRESULT(hr, severity, LOCATION, format, __VA_ARGS__)
#define LOG_HRESULT_IF_FAILED(hr, action, severity, format, ...) IF(FAILED(hr), LOG_HRESULT(hr, severity, format, __VA_ARGS__); action)

void
LogLastError(Severity severity, Location location, c8* message)
{
	u32 lastError = GetLastError();
	LogFormatMessage(lastError, severity, location, message);
}

template<typename... Args>
inline void
LogLastError(Severity severity, Location location, c8* format, Args... args)
{
	String message = {};
	defer { List_Free(message); };

	b32 success = String_Format(message, format, args...);
	if (!success)
	{
		LogLastError(severity, location, format);
		return;
	}

	LogLastError(severity, location, message.data);
}
#define LOG_LAST_ERROR(severity, format, ...) LogLastError(severity, LOCATION, format, __VA_ARGS__)
#define LOG_LAST_ERROR_IF(expression, action, severity, format, ...) IF(expression, LOG_LAST_ERROR(severity, format, __VA_ARGS__); action)

// TODO: RAII wrapper
static String
GetWorkingDirectory()
{
	String result = {};

	u32 length = GetCurrentDirectoryA(0, nullptr);
	LOG_LAST_ERROR_IF(!length, return result,
		Severity::Warning, "Failed to get working directory length");

	b32 success = List_Reserve(result, length);
	LOG_IF(!success, return result,
		Severity::Warning, "Failed to allocate working directory");

	u32 written = GetCurrentDirectoryA(length + 1, result.data);
	LOG_LAST_ERROR_IF(!written, return result,
		Severity::Warning, "Failed to get working directory");
	LOG_IF(written != length, return result,
		Severity::Warning, "Working directory length does not match expected");

	result.length = length + 1;
	return result;
}

static Bytes
LoadFile(c8* path, u32 padding = 0)
{
	b32 success;
	Bytes result = {};
	{
		auto resultGuard = guard { List_Free(result); };

		HANDLE file = CreateFileA(
			path,
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			nullptr
		);
		defer { CloseHandle(file); };
		if (file == INVALID_HANDLE_VALUE)
		{
			String cwd = GetWorkingDirectory();
			defer { List_Free(cwd); };
			LOG_LAST_ERROR(Severity::Warning, "Failed to create file handle '%s'; CWD: '%s'", path, cwd);
			return result;
		}

		LARGE_INTEGER size_win32;
		success = GetFileSizeEx(file, &size_win32);
		LOG_LAST_ERROR_IF(!success, return result,
			Severity::Warning, "Failed to get file size '%s'", path);
		LOG_IF(size_win32.QuadPart > u32Max - padding, return result,
			Severity::Warning, "File is too large to load '%s'", path);

		u32 size = (u32) size_win32.QuadPart;
		success = List_Reserve(result, size + padding);
		LOG_IF(!success, return result,
			Severity::Warning, "Failed to allocate file memory '%s'", path);

		success = ReadFile(file, result.data, size, (DWORD*) &result.length, nullptr);
		LOG_LAST_ERROR_IF(!success, return result,
			Severity::Warning, "Failed to read file '%s'", path);

		resultGuard.dismiss = true;
	}
	return result;
}

Bytes
Platform_LoadFileBytes(c8* path)
{
	return LoadFile(path, 0);
}

String
Platform_LoadFileString(c8* path)
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

u64
Platform_GetTicks()
{
	// TODO: Ensure we're not on XP
	// NOTE: Never fails above XP
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return (u64) counter.QuadPart;
}

r32
Platform_TicksToSeconds(i64 ticks)
{
	// TODO: Ensure we're not on XP
	// NOTE: Never fails above XP
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return (r32) ticks / (r32) frequency.QuadPart;
}

r32
Platform_GetElapsedSeconds(u64 startTicks)
{
	i64 elapsedTicks = (i64) (Platform_GetTicks() - startTicks);
	return Platform_TicksToSeconds(elapsedTicks);
}

struct PipeImpl
{
	HANDLE     handle;
	OVERLAPPED overlapped;
};

b32
Platform_CreatePipe(StringSlice name, Pipe* pipe)
{
	*pipe = {};

	pipe->impl = (PipeImpl*) malloc(sizeof(PipeImpl));
	LOG_IF(!pipe->impl, return false,
		Severity::Warning, "Failed to allocate pipe");

	auto cleanupGuard = guard{
		CloseHandle(pipe->impl->handle);
		CloseHandle(pipe->impl->overlapped.hEvent);
		free(pipe->impl);
		*pipe = {};
	};

	String fullName = {};
	defer { List_Free(fullName); };

	b32 success = String_Format(fullName, "\\\\.\\pipe\\%s", name.data);
	LOG_IF(!success, return false,
		Severity::Warning, "Failed to format string for pipe name");

	// TODO: Enforce a single instance of the simulation
	pipe->impl->handle = CreateNamedPipeA(
		fullName.data,
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
		1,
		// TODO: See remarks
		1024, 1024,
		0,
		nullptr
	);
	LOG_LAST_ERROR_IF(pipe->impl->handle == INVALID_HANDLE_VALUE, return false,
		Severity::Warning, "Failed to create pipe '%s'", name.data);

	pipe->impl->overlapped.hEvent = CreateEventA(nullptr, true, true, nullptr);
	LOG_LAST_ERROR_IF(pipe->impl->overlapped.hEvent == INVALID_HANDLE_VALUE, return false,
		Severity::Warning, "Failed to create pipe event '%s'", name.data);

	cleanupGuard.dismiss = true;
	return true;
}

static inline b32
IsValidHandle(HANDLE handle)
{
	// NOTE: This is a nasty bit of shit design. Some Win32 functions return nullptr and some return
	// INVALID_HANDLE_VALUE on failure. Things would be fine if that were the end of it, but it
	// isn't. Some functions take 'pseudohandles' and INVALID_HANDLE_VALUE can be a valid
	// pseudohandle. Fuck you, Microsoft.
	return handle != nullptr && handle != INVALID_HANDLE_VALUE;
}

#if false
void
Platform_DestroyPipe(Pipe* pipe)
{
	if (IsValidHandle(pipe->impl->handle))
	{
		// TODO: Handle failure?
		FlushFileBuffers(pipe->impl->handle);
		DisconnectNamedPipe(pipe->impl->handle);
	}

	CloseHandle(pipe->impl->handle);
	CloseHandle(pipe->impl->event);
	free(pipe->impl);
	*pipe = {};
}
#endif

template<typename T>
b32
Platform_WritePipe(Pipe* pipe, T data)
{
	Slice<u8> bytes = {};
	bytes.length = sizeof(T);
	bytes.stride = 1;
	bytes.data   = (u8*) &data;
	return Platform_WritePipe(pipe, bytes);
}

b32
Platform_WritePipe(Pipe* pipe, Slice<u8> bytes)
{
	// NOTE: Writes are synchronous. If this changes be aware of the following facts: 1) The
	// 'written' parameter is not valid for async writes and 2) the data buffer MUST NOT CHANGE
	// during an async write.

	// NOTE: Non-overlapped writes will only block once the buffer is full.

	// TODO: Try doing overlapped io with a non-overlap flagged pipe
	// TODO: It seems like doing this write non-overlapped is the right thing to do. However, we need
	// to be sure that we can't end up in a state where both the sim and the gui are doing blocking
	// writes. My instinct is that this can happen any time one of them writes twice without a read
	// in between.

	b32 success;

	//u32 written;
	success = WriteFile(
		pipe->impl->handle,
		bytes.data,
		bytes.length,

		// Overlapped
		nullptr,
		&pipe->impl->overlapped

		// Non-Overlapped
		//(LPDWORD) &written,
		//nullptr
	);

	if (!success)
	{
		// ERROR_INVALID_USER_BUFFER Full
		// ERROR_NOT_ENOUGH_MEMORY   Full
		// ERROR_OPERATION_ABORTED   Canceled
		// ERROR_NOT_ENOUGH_QUOTA    Buffer could not be page-locked

		u32 error = GetLastError();
		switch (error)
		{
			// Receiver has not connected. This is ok.
			case ERROR_PIPE_LISTENING: return false;

			// The write is overlapped. This is ok.
			case ERROR_IO_PENDING: break;

			default:
			{
				// NOTE: Just in case. GetLastError isn't documented as preserving the error.
				SetLastError(error);
				LOG_LAST_ERROR_IF(!success, return false,
					Severity::Warning, "Starting write to pipe failed");
				break;
			}
		}
	}

	//success = GetOverlappedResult(
	//	pipe->handle,
	//	&pipe->overlapped,
	//	(LPDWORD) &written,
	//	true
	//);
	//LOG_LAST_ERROR_IF(!success, return PipeResult::Failure,
	//	Severity::Warning, "Writing to pipe failed");

	// NOTE: This is a severe error, since it leaves the reader in an unpredictably corrupted state.
	// However, we control both the reader and the writer and can account for this at a higher level.
	// The simulation includes the size in a fixed size header at the beginning of each message. If
	// the reader gets a message smaller than the header size it can ignore it. If the reader gets a
	// message that is bigger than the header but doesn't match the expected size it can ignore it.
	// The sender will get a failure message and simply resend the message next time it gets a
	// chance.
	//LOG_IF(written != bytes.length, return false,
	//	Severity::Fatal, "Writing to pipe truncated");

	return true;
}
