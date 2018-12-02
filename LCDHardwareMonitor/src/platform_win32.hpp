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

	Platform_Print(message);
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

	Platform_Print(fullMessage);
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

	Platform_Log(severity, location, message);
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

	Platform_Log(severity, location, "%s: %s", message, windowsMessage);
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

	LogHRESULT(hr, severity, location, message);
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

	LogLastError(severity, location, message);
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

	List_Reserve(result, length);
	LOG_IF(!result, return result,
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
			LOG_LAST_ERROR(Severity::Warning, "Failed to create file handle '%s'; CWD: '%s'", path, cwd.data);
			return result;
		}

		LARGE_INTEGER size_win32;
		success = GetFileSizeEx(file, &size_win32);
		LOG_LAST_ERROR_IF(!success, return result,
			Severity::Warning, "Failed to get file size '%s'", path);
		LOG_IF(size_win32.QuadPart > u32Max - padding, return result,
			Severity::Warning, "File is too large to load '%s'", path);

		u32 size = (u32) size_win32.QuadPart;
		List_Reserve(result, size + padding);
		LOG_IF(!result, return result,
			Severity::Warning, "Failed to allocate file memory '%s'", path);

		success = ReadFile(file, result, size, (DWORD*) &result.length, nullptr);
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
	if (!bytes) return result;

	result.length   = bytes.length;
	result.capacity = bytes.capacity;
	result.data     = (c8*) bytes.data;

	result.data[result.length++] = '\0';

	return result;
}

u64 Platform_GetTicks()
{
	// TODO: Ensure we're not on XP
	// NOTE: Never fails above XP
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);

	return (u64) counter.QuadPart;
}

r32 Platform_TicksToSeconds(i64 ticks)
{
	// TODO: Ensure we're not on XP
	// NOTE: Never fails above XP
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return (r32) ticks / (r32) frequency.QuadPart;
}

r32 Platform_GetElapsedSeconds(u64 startTicks)
{
	i64 elapsedTicks = (i64) (Platform_GetTicks() - startTicks);
	return Platform_TicksToSeconds(elapsedTicks);
}
