inline void
Platform_Print(c8* message)
{
	// NOTE: printf will not go to Visual Studio's Output window :(
	printf(message);
	OutputDebugStringA(message);
}

void
Platform_Log(c8* message, Severity severity, c8* file, i32 line, c8* function)
{
	String fullMessage = {};
	b32 success = String_Format(fullMessage, "%s - %s\n\t%s(%i)\n", function, message, file, line);
	if (!success)
	{
		Platform_Print("String_Format failed. Original message:\n\t");
		Platform_Print(function);
		Platform_Print(" - ");
		Platform_Print(message);
		Platform_Print("\n\t\t");
		Platform_Print(file);
		Platform_Print("\n");
		return;
	}
	defer { List_Free(fullMessage); };

	Platform_Print(fullMessage);

	if (severity > Severity::Info)
		__debugbreak();
}

static void
LogFormatMessage(c8* message, Severity severity, u32 messageID, c8* file, i32 line, c8* function)
{
	c8* windowsMessage = 0;
	u32 uResult = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		nullptr,
		messageID,
		0,
		(c8*) &windowsMessage,
		0,
		nullptr
	);
	defer { LocalFree(windowsMessage); };
	if (uResult == 0)
	{
		Platform_Print("FormatMessageA failed. Original message:\n\t");
		Platform_Print(function);
		Platform_Print(" - ");
		Platform_Print(message);
		Platform_Print("\n\t\t");
		Platform_Print(file);
		Platform_Print("\n");
		return;
	}

	String fullMessage = {};
	b32 success = String_Format(
		fullMessage,
		"%s: %s - %s\n\t%s(%i)\n",
		message,
		windowsMessage,
		function,
		message,
		file,
		line
	);
	if (!success)
	{
		Platform_Print("String_Format failed. Original message:\n\t");
		Platform_Print(function);
		Platform_Print(" - ");
		Platform_Print(message);
		Platform_Print(": ");
		Platform_Print(windowsMessage);
		Platform_Print("\n\t\t");
		Platform_Print(file);
		Platform_Print("\n");
		return;
	}
	defer { List_Free(fullMessage); };

	Platform_Print(fullMessage);

	if (severity > Severity::Info)
		__debugbreak();
}

void
LogHRESULT(c8* message, Severity severity, HRESULT hr, c8* file, i32 line, c8* function)
{
	LogFormatMessage(message, severity, (u32) hr, file, line, function);
}
#define LOG_HRESULT(message, severity, hr) LogHRESULT(message, severity, hr, LOCATION_ARGS)
#define LOG_HRESULT_IF_FAILED(hr, message, severity, ...) IF(FAILED(hr), LOG_HRESULT(message, severity, hr); __VA_ARGS__)

void
LogLastError(c8* message, Severity severity, c8* file, i32 line, c8* function)
{
	u32 lastError = GetLastError();
	LogFormatMessage(message, severity, lastError, file, line, function);
}
#define LOG_LAST_ERROR(message, severity) LogLastError(message, severity, LOCATION_ARGS)
#define LOG_LAST_ERROR_IF(expression, message, severity, ...) IF(expression, LOG_LAST_ERROR(message, severity); __VA_ARGS__)

static Bytes
LoadFile(c8* fileName, u32 padding = 0)
{
	// TODO: Add cwd to errors

	b32 success;
	Bytes result = {};
	{
		auto resultGuard = guard { List_Free(result); };

		HANDLE file = CreateFileA(
			fileName,
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
			nullptr
		);
		LOG_LAST_ERROR_IF(file == INVALID_HANDLE_VALUE, "CreateFileA failed", Severity::Warning, return result);
		defer { CloseHandle(file); };

		LARGE_INTEGER size_win32;
		success = GetFileSizeEx(file, &size_win32);
		LOG_LAST_ERROR_IF(!success, "GetFileSizeEx failed", Severity::Warning, return result);
		LOG_IF(size_win32.QuadPart > u32Max - padding, "File is too large", Severity::Warning, return result);

		u32 size = (u32) size_win32.QuadPart;
		List_Reserve(result, size + padding);
		LOG_IF(!result, "Failed to allocate file memory", Severity::Warning, return result);

		success = ReadFile(file, result, size, (DWORD*) &result.length, nullptr);
		LOG_LAST_ERROR_IF(!success, "ReadFile failed", Severity::Warning, return result);

		resultGuard.dismiss = true;
	}
	return result;
}

Bytes
Platform_LoadFileBytes(c8* fileName)
{
	return LoadFile(fileName, 0);
}

String
Platform_LoadFileString(c8* fileName)
{
	String result = {};

	Bytes bytes = LoadFile(fileName, 1);
	if (!bytes) return result;

	result.length   = bytes.length;
	result.capacity = bytes.capacity;
	result.data     = (c8*) bytes.data;

	result.data[result.length++] = '\0';

	return result;
}
