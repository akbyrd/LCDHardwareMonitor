//
// Logging
//

inline void
ConsolePrint(c16* message)
{
	OutputDebugStringW(message);
}

#define LOG_HRESULT(message, severity, hr) LogHRESULT(message, severity, hr, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__))
void
LogHRESULT(c16* message, Severity severity, HRESULT hr, c16* file, i32 line, c16* function)
{
	Log(message, severity, file, line, function);
}

#define LOG_LAST_ERROR(message, severity) LogLastError(message, severity, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__))
#define LOG_LAST_ERROR_IF(expression, message, severity, ...) IF(expression, LOG_LAST_ERROR(message, severity); __VA_ARGS__)
b32
LogLastError(c16* message, Severity severity, c16* file, i32 line, c16* function)
{
	c16 windowsMessage[256];
	u32 uResult = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		GetLastError(),
		0,
		windowsMessage,
		ArrayCount(windowsMessage),
		nullptr
	);
	LOG_LAST_ERROR_IF(uResult == 0, L"FormatMessage failed", Severity::Warning);

	//TODO: Check for swprintf failure (overflow).
	c16 combinedMessage[256];
	swprintf(combinedMessage, ArrayCount(combinedMessage), L"%s: %s", message, windowsMessage);

	Log(combinedMessage, severity, file, line, function);

	return true;
}

//
// File handling
//

#include <fstream>
using namespace std;

b32
LoadFile(c16* fileName, unique_ptr<c8[]>& data, u64& dataSize)
{
	//TODO: Add cwd to error
	ifstream inFile(fileName, ios::binary | ios::ate);
	if (!inFile.is_open())
	{
		//LOG(L"Failed to open file: " + fileName, Severity::Error);
		LOG(L"Failed to open file: ", Severity::Error);
		return false;
	}

	dataSize = (size_t) inFile.tellg();
	data = make_unique<char[]>(dataSize);

	inFile.seekg(0);
	inFile.read(data.get(), dataSize);

	if (inFile.bad())
	{
		dataSize = 0;
		data = nullptr;

		//LOG(L"Failed to read file: " + fileName, Severity::Error);
		LOG(L"Failed to read file: ", Severity::Error);
		return false;
	}

	return true;
}