//
// Logging
//

void
Platform_Log(c16* message, Severity severity, c16* file, i32 line, c16* function)
{
	//TODO: Decide on logging allocation policy
	//TODO: Check for swprintf failure (overflow).
	c16 buffer[512];
	swprintf(buffer, ArrayLength(buffer), L"%s - %s\n\t%s(%i)\n", function, message, file, line);
	Platform_Print(buffer);

	if (severity > Severity::Info)
		__debugbreak();
}

inline void
Platform_Print(c16* message)
{
	OutputDebugStringW(message);
}

static void
LogFormatMessage(c16* message, Severity severity, u32 messageID, c16* file, i32 line, c16* function)
{
	c16 windowsMessage[256];
	u32 uResult = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		messageID,
		0,
		windowsMessage,
		ArrayLength(windowsMessage) - 1,
		nullptr
	);

	//NOTE: If we fail this far down, just...fuck it.
	c16 combinedMessage[256];
	if (uResult == 0)
	{
		i32 iResult = swprintf(combinedMessage, ArrayLength(combinedMessage), L"%s: %i", message, messageID);
		if (iResult < 0)
		{
			Platform_Log(L"swprintf failed", Severity::Warning, WFILE, LINE, WFUNC);
			Platform_Log(message, severity, file, line, function);
		}
	}
	else
	{
		windowsMessage[uResult - 1] = '\0';
		i32 iResult = swprintf(combinedMessage, ArrayLength(combinedMessage), L"%s: %s", message, windowsMessage);
		if (iResult < 0)
		{
			Platform_Log(L"swprintf failed", Severity::Warning, WFILE, LINE, WFUNC);
			Platform_Log(message, severity, file, line, function);
		}
	}

	Platform_Log(combinedMessage, severity, file, line, function);
}

void
LogHRESULT(c16* message, Severity severity, HRESULT hr, c16* file, i32 line, c16* function)
{
	LogFormatMessage(message, severity, hr, file, line, function);
}
#define LOG_HRESULT(message, severity, hr) LogHRESULT(message, severity, hr, WFILE, LINE, WFUNC)
#define LOG_HRESULT_IF_FAILED(hr, message, severity, ...) IF(FAILED(hr), LOG_HRESULT(message, severity, hr); __VA_ARGS__)

void
LogLastError(c16* message, Severity severity, c16* file, i32 line, c16* function)
{
	u32 lastError = GetLastError();
	LogFormatMessage(message, severity, lastError, file, line, function);
}
#define LOG_LAST_ERROR(message, severity) LogLastError(message, severity, WFILE, LINE, WFUNC)
#define LOG_LAST_ERROR_IF(expression, message, severity, ...) IF(expression, LOG_LAST_ERROR(message, severity); __VA_ARGS__)


//
// File handling
//

#include <fstream>

static Bytes
LoadFile(c16* fileName, i32 padding = 0)
{
	Assert(padding >= 0);

	//TODO: Add cwd to errors
	//TODO: Handle files larger than 4 GB
	//TODO: Handle c8/c16/void

	Bytes result = {};

	std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);
	LOG_IF(!inFile.is_open(), L"Failed to open file: <file>. Working Directory: <cwd>", Severity::Error, goto Cleanup);

	result.length   = (i32) inFile.tellg();
	result.capacity = result.length + padding;
	LOG_IF(i32Max - result.length < padding, L"File is too big to fit requested padding when loading: <file>", Severity::Error, goto Cleanup);

	result.data = (u8*) malloc(sizeof(u8) * result.capacity);

	inFile.seekg(0);
	LOG_IF(inFile.fail(), L"Failed to seek file: <file>", Severity::Error, goto Cleanup);
	inFile.read((c8*) result.data, result.length);
	LOG_IF(inFile.fail(), L"Failed to read file: <file>", Severity::Error, goto Cleanup);

	return result;

Cleanup:
	List_Free(result);
	return result;
}

Bytes
Platform_LoadFileBytes(c16* fileName)
{
	return LoadFile(fileName, 0);
}

String
Platform_LoadFileString(c16* fileName)
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

WString
Platform_LoadFileWString(c16* fileName)
{
	WString result = {};
	Bytes bytes = LoadFile(fileName, 1);

	result.length   = bytes.length;
	result.capacity = bytes.capacity;
	result.data     = (c16*) bytes.data;

	result.data[result.length++] = '\0';

	return result;
}
