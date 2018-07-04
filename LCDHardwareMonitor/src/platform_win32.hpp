// TODO: Remove this
#include <fstream>

void
Platform_Log(c8* message, Severity severity, c8* file, i32 line, c8* function)
{
	// TODO: Dynamic allocation
	// TODO: Check for snprintf failure (overflow).
	c8 buffer[512];
	snprintf(buffer, ArrayLength(buffer), "%s - %s\n\t%s(%i)\n", function, message, file, line);
	Platform_Print(buffer);

	if (severity > Severity::Info)
		__debugbreak();
}

inline void
Platform_Print(c8* message)
{
	OutputDebugStringA(message);
}

static void
LogFormatMessage(c8* message, Severity severity, u32 messageID, c8* file, i32 line, c8* function)
{
	// TODO: Dynamic allocation
	c8 windowsMessage[256];
	u32 uResult = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		messageID,
		0,
		windowsMessage,
		ArrayLength(windowsMessage) - 1,
		nullptr
	);

	// NOTE: If we fail this far down, just...fuck it.
	// TODO: Dynamic allocation
	c8 combinedMessage[256];
	if (uResult == 0)
	{
		i32 iResult = snprintf(combinedMessage, ArrayLength(combinedMessage), "%s: %i", message, messageID);
		if (iResult < 0)
		{
			Platform_Log("snprintf failed", Severity::Warning, LOCATION_ARGS);
			Platform_Log(message, severity, file, line, function);
		}
	}
	else
	{
		windowsMessage[uResult - 1] = '\0';
		i32 iResult = snprintf(combinedMessage, ArrayLength(combinedMessage), "%s: %s", message, windowsMessage);
		if (iResult < 0)
		{
			Platform_Log("snprintf failed", Severity::Warning, LOCATION_ARGS);
			Platform_Log(message, severity, file, line, function);
		}
	}

	Platform_Log(combinedMessage, severity, file, line, function);
}

void
LogHRESULT(c8* message, Severity severity, HRESULT hr, c8* file, i32 line, c8* function)
{
	LogFormatMessage(message, severity, hr, file, line, function);
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
LoadFile(c8* fileName, i32 padding = 0)
{
	Assert(padding >= 0);

	// TODO: Add cwd to errors
	// TODO: Handle files larger than 4 GB
	// TODO: Handle c8/void

	Bytes result = {};

	std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);
	LOG_IF(!inFile.is_open(), "Failed to open file: <file>. Working Directory: <cwd>", Severity::Error, goto Cleanup);

	result.length   = (i32) inFile.tellg();
	result.capacity = result.length + padding;
	LOG_IF(i32Max - result.length < padding, "File is too big to fit requested padding when loading: <file>", Severity::Error, goto Cleanup);

	result.data = (u8*) malloc(sizeof(u8) * result.capacity);

	inFile.seekg(0);
	LOG_IF(inFile.fail(), "Failed to seek file: <file>", Severity::Error, goto Cleanup);
	inFile.read((c8*) result.data, result.length);
	LOG_IF(inFile.fail(), "Failed to read file: <file>", Severity::Error, goto Cleanup);

	return result;

Cleanup:
	List_Free(result);
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
