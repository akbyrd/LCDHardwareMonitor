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
void
LogLastError(c16* message, Severity severity, c16* file, i32 line, c16* function)
{
	i32 error = GetLastError();
	Log(message, severity, file, line, function);
}

#define LOG_LAST_ERROR_IF(expression, string, reaction) \
if (expression)                                         \
{                                                       \
	LOG_LAST_ERROR(string, Severity::Error);            \
	reaction;                                           \
}                                                       \


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