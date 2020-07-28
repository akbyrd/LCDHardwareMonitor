#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstdint>
#include <stdarg.h>

using u8  = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long int;

#define Assert(condition) if (!(condition)) { *((int*) 0) = 0; }
#define BYTE(n, val) u8(((val) >> ((n)*8)) & 0xFF)

template<typename T, size_t S>
constexpr inline size_t ArrayLength(const T(&)[S]) { return S; }

void Print(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[1024];
	vsnprintf(buffer, ArrayLength(buffer), format, args);
	va_end(args);

	OutputDebugStringA(buffer);
}

void PrintBytes(const char* prefix, u8* buffer, u16 bufferLen)
{
	Print("%s", prefix);
	for (int i = 0; i < bufferLen; i++)
		Print(" 0x%.2X", buffer[i]);
	Print("\n");
}

template<u16 N>
void PrintBytes(const char* prefix, u8 (&buffer)[N])
{
	Print("%s", prefix);
	for (int i = 0; i < N; i++)
		Print(" 0x%.2X", buffer[i]);
	Print("\n");
}

void PrintBytesRaw(u8* buffer, u16 bufferLen)
{
	if (bufferLen > 0)
	{
		Print("0x%.2X", buffer[0]);
		for (int i = 1; i < bufferLen; i++)
			Print(" 0x%.2X", buffer[i]);
	}
}

template<u16 N>
void PrintBytesRaw(u8(&buffer)[N])
{
	PrintBytesRaw(buffer, N);
}

#define ENABLE_TRACE false
#if ENABLE_TRACE
	#define Trace(...) Print(__VA_ARGS__)
	#define TraceBytes(...) PrintBytes(__VA_ARGS__)
	#define TraceRaw(...) PrintBytesRaw(__VA_ARGS__)
#else
	#define Trace(...)
	#define TraceBytes(...)
	#define TraceRaw(...)
#endif

template <typename T>
T Min(T a, T b) { return a <= b ? a : b; }

template <typename T>
T Max(T a, T b) { return a >= b ? a : b; }

u64 GetTime()
{
	LARGE_INTEGER time;
	bool success = QueryPerformanceCounter(&time);
	Assert(success);

	return time.QuadPart;
}

float GetElapsedMs(u64 since)
{
	u64 now = GetTime();
	LARGE_INTEGER frequency;
	bool success = QueryPerformanceFrequency(&frequency);
	Assert(success);
	return float(double(now - since) / double(frequency.QuadPart) * 1000.0);
}

enum struct Signal
{
	Low = 0,
	High = 1,
};

constexpr u16 Color16(u8 r, u8 g, u8 b)
{
	u16 rp = (r & 0b1111'1000) << 8;
	u16 gp = (g & 0b1111'1100) << 3;
	u16 bp = (b & 0b1111'1000) >> 3;
	u16 color = rp | gp | bp;
	return color;
}

struct Color
{
	static const u16 Black   = Color16(0x00, 0x00, 0x00);
	static const u16 Gray    = Color16(0x80, 0x80, 0x80);
	static const u16 White   = Color16(0xFF, 0xFF, 0xFF);
	static const u16 Red     = Color16(0xFF, 0x00, 0x00);
	static const u16 Green   = Color16(0x00, 0xFF, 0x00);
	static const u16 Blue    = Color16(0x00, 0x00, 0xFF);
	static const u16 Cyan    = Color16(0x00, 0xFF, 0xFF);
	static const u16 Magenta = Color16(0xFF, 0x00, 0xFF);
	static const u16 Yellow  = Color16(0xFF, 0xFF, 0x00);
};

#define Unpack2(x) (((x) >> 8) & 0xFF), ((x) & 0xFF)
#define Unpack3(x) (((x) >> 16) & 0xFF), (((x) >> 8) & 0xFF), ((x) & 0xFF)
#define Unpack4(x) (((x) >> 24) & 0xFF), (((x) >> 16) & 0xFF), (((x) >> 8) & 0xFF), ((x) & 0xFF)

#include "FT232H.h"
#include "ILI9341.h"

int main()
{
	FT232H::State ft232h = {};
	ILI9341::State ili9341 = {};

	FT232H_Initialize(&ft232h);
	FT232H_Write(&ft232h, FT232H::Command::SendImmediate);
	ILI9341_Initialize(&ili9341, &ft232h);

#if true
	// looks correct, matches python
	// 0x00 0x00 0x00 0x00
	ILI9341_WriteCmd(&ili9341, ILI9341::Command::ReadDisplayIdentificationInformation);
	//FT232H_WaitForResponse(&ft232h);
	//FT232H_SetDC(&ft232h, Signal::High); // TODO: Doesn't seem necessary?
	u8 deviceInfoResponse[4];
	FT232H_Read(&ft232h, deviceInfoResponse);
	PrintBytes("Device Info", deviceInfoResponse);
#endif

#if true
	// looks correct, matches python
	// 0xD2 0x29 0x82 0x00 0x00
	ILI9341_WriteCmd(&ili9341, ILI9341::Command::ReadDisplayStatus);
	u8 deviceStatusResponse[5];
	FT232H_Read(&ft232h, deviceStatusResponse);
	PrintBytes("Device Status", deviceStatusResponse);
#endif

#if true
	// looks correct, matches python (second byte is a bit off?)
	// 0x9C 0x01
	ILI9341_WriteCmd(&ili9341, ILI9341::Command::ReadDisplayPowerMode);
	u8 powerModeResponse[2];
	//FT232H_SetDC(&ft232h, Signal::High);
	FT232H_Read(&ft232h, powerModeResponse);
	PrintBytes("Power Mode", powerModeResponse);
#endif

#if true
	// looks correct, matches python (second byte is a bit off?)
	// 0x48 0x01
	ILI9341_WriteCmd(&ili9341, ILI9341::Command::ReadDisplayMADCTL);
	u8 madctlResponse[2];
	FT232H_Read(&ft232h, madctlResponse);
	PrintBytes("MADCTL", madctlResponse);
#endif

#if true
	// looks correct, matches python
	// 0x05 0xFF
	ILI9341_WriteCmd(&ili9341, ILI9341::Command::ReadDisplayPixelFormat);
	u8 pixelFormatResponse[2];
	FT232H_Read(&ft232h, pixelFormatResponse);
	PrintBytes("Pixel Format", pixelFormatResponse);
#endif

#if true
	// looks correct, matches python (second byte is a bit off?)
	// 0x00 0x01
	ILI9341_WriteCmd(&ili9341, ILI9341::Command::ReadDisplayImageFormat);
	u8 imageFormatResponse[2];
	FT232H_Read(&ft232h, imageFormatResponse);
	PrintBytes("Image Format", imageFormatResponse);
#endif

#if true
	// matches python (second byte is a bit off?)
	// 0x00 0x01
	ILI9341_WriteCmd(&ili9341, ILI9341::Command::ReadDisplaySignalMode);
	u8 signalModeResponse[2];
	FT232H_Read(&ft232h, signalModeResponse);
	PrintBytes("Signal Mode", signalModeResponse);
#endif

#if true
	// looks correct, matches python (second byte is a bit off?)
	// 0xC0 0x01
	ILI9341_WriteCmd(&ili9341, ILI9341::Command::ReadDisplaySelfDiagnosticResult);
	u8 diagnosticResponse[2];
	FT232H_Read(&ft232h, diagnosticResponse);
	PrintBytes("Diagnostic", diagnosticResponse);
#endif

	ILI9341_Finalize(&ili9341);
	FT232H_Finalize(&ft232h);

	return 0;
}

// TODO: Figure out why brightness is wonky with both versions (setting?)
// TODO: Check ReadDisplayStatus values
