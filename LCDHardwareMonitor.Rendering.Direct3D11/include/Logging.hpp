#pragma once

#include <string>
#include <sstream>

#define LOG(msg)           Logging::Log       (msg, __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING(msg) { Logging::LogWarning(msg, __FILE__, __LINE__, __FUNCTION__); __debugbreak(); }
#define LOG_ERROR(msg)   { Logging::LogError  (msg, __FILE__, __LINE__, __FUNCTION__); __debugbreak(); }
#define LOG_HRESULT(hr)    Logging::LogHRESULT( hr, __FILE__, __LINE__, __FUNCTION__)
#define LOG_LASTERROR()    LOG_HRESULT(HRESULT_FROM_WIN32(GetLastError()))
//TODO: Log expression


//TODO: Make implementation DRYer
namespace Logging
{
	void Log(wstring message)
	{
		wostringstream stream;

		stream << message << endl;

		//Send it all to the VS Output window
		OutputDebugStringW(stream.str().c_str());
	}

	void Log(wostringstream& stream)
	{
		stream << endl;

		//Send it all to the VS Output window
		OutputDebugStringW(stream.str().c_str());
	}

	void Log(wstring message, const char* file, long line, const char* function)
	{
		wostringstream stream;

		stream << function << L" - " << message << endl;

		//Append the error location (it's clickable!)
		stream << L"\t" << file << L"(" << line << L")" << endl;

		//Send it all to the VS Output window
		OutputDebugStringW(stream.str().c_str());
	}

	void LogWarning(wstring message, const char* file, long line, const char* function)
	{
		wostringstream stream;

		stream << L"WARNING: " << function << L" - " << message << endl;

		//Append the error location (it's clickable!)
		stream << L"\t" << file << L"(" << line << L")" << endl;

		//Send it all to the VS Output window
		OutputDebugStringW(stream.str().c_str());

		//__debugbreak();
	}

	void LogError(wstring message, const char* file, long line, const char* function)
	{
		wostringstream stream;

		stream << L"ERROR: " << function << L" - " << message << endl;

		//Append the error location (it's clickable!)
		stream << "\t" << file << L"(" << line << L")" << endl;

		//Send it all to the VS Output window
		OutputDebugStringW(stream.str().c_str());

		//__debugbreak();
	}

	void LogAssert(wstring expression, wstring message, const char* file, long line, const char* function)
	{
		wostringstream stream;

		stream << L"ASSERT FAILED: " << function << L" - " << expression << endl
			<< message << endl;

		//Append the error location (it's clickable!)
		stream << L"\t" << file << L"(" << line << L")" << endl;

		//Send it all to the VS Output window
		OutputDebugStringW(stream.str().c_str());

		__debugbreak();
	}

	bool LogHRESULT(HRESULT hr, const char* file, long line, const char* function)
	{
		if (!FAILED(hr)) {return false;}

		//Get a friendly string from the HRESULT
		wchar_t* errorMessage;
		DWORD ret = FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			nullptr,
			hr,
			LANG_USER_DEFAULT,
			(LPTSTR) &errorMessage,
			0,
			nullptr
		);

		wostringstream stream;

		//Log the friendly error message
		if (ret)
		{
			stream << L"ERROR: " << function << L" - " << errorMessage << endl;

			if (!HeapFree(GetProcessHeap(), NULL, errorMessage))
			{
				stream << L"Failed to HeapFree error buffer. Error: " << GetLastError();
			}
		}
		//FormatMessage failed, log that too
		else
		{
			stream << L"ERROR: " << function << L" - Failed to format error message from HRESULT: "
				<< hr << L". FormatMessage error: " << GetLastError() << endl;
		}

		//Append the error location (it's clickable!)
		stream << L"\t" << file << L"(" << line << L")" << endl;

		//Send it all to the VS Output window
		OutputDebugStringW(stream.str().c_str());

		__debugbreak();
		return true;
	}
}