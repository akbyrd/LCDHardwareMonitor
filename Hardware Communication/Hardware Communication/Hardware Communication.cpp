#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstdint>
#include <stdarg.h>

using u8 = unsigned char;
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

#define ENABLE_TRACE false
#if ENABLE_TRACE
	#define Trace(...) Print(__VA_ARGS__)
#else
	#define Trace(...)
#endif

template <typename T>
T Min(T a, T b) { return a <= b ? a : b; }

template<u16 N>
void TraceBytes(const char* prefix, u8 (&buffer)[N])
{
#if ENABLE_TRACE || true
	Print("%-14s", prefix);
	for (int i = 0; i < N; i++)
		Print(" 0x%.2X", buffer[i]);
	Print("\n");
#endif
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
	ILI9341_Initialize(&ili9341, &ft232h);

	ILI9341_Clear(&ili9341, Color::White);

	ILI9341_SetPixel(&ili9341,   0 - 0,   0 - 0, Color::Red);   // TL
	ILI9341_SetPixel(&ili9341, 240 - 1,   0 - 0, Color::Green); // TR
	ILI9341_SetPixel(&ili9341,   0 - 0, 320 - 1, Color::Blue);  // BL
	ILI9341_SetPixel(&ili9341, 240 - 1, 320 - 1, Color::White); // BR

	for (int i = 0; i < 120; i++)
	{
		ILI9341_SetPixel(&ili9341,   0 - 0 + i,   0 - 0 + i, Color::Red);
		ILI9341_SetPixel(&ili9341, 240 - 1 - i,   0 - 0 + i, Color::Green);
		ILI9341_SetPixel(&ili9341,   0 - 0 + i, 320 - 1 - i, Color::Blue);
		ILI9341_SetPixel(&ili9341, 240 - 1 - i, 320 - 1 - i, Color::Gray);
	}

	for (int i = 0; i < 80; i++)
	{
		ILI9341_SetPixel(&ili9341, 240 / 2 - 0, 120 + i, Color::Black);
		ILI9341_SetPixel(&ili9341, 240 / 2 - 1, 120 + i, Color::Black);
	}

	ILI9341_SetRect(&ili9341, 120 - 10,  60 - 10, 120 + 10,  60 + 10, Color::Yellow);
	ILI9341_SetRect(&ili9341,  60 - 10, 160 - 10,  60 + 10, 160 + 10, Color::Magenta);
	ILI9341_SetRect(&ili9341, 180 - 10, 160 - 10, 180 + 10, 160 + 10, Color::Gray);
	ILI9341_SetRect(&ili9341, 120 - 10, 260 - 10, 120 + 10, 260 + 10, Color::Cyan);

	ILI9341_Finalize(&ili9341);
	FT232H_Finalize(&ft232h);
	return 0;
}

// Features for the final version
// 	Ability to trivially toggle tracing
// 	Ability to trivially toggle write batching
// 	Ability to profile time spend sleeping
// 	Commands take a slice

// TODO: Confirm reads are working correctly at all (pixel format?)
// TODO: Figure out why consecutive reads don't seem to work
// TODO: Figure out why brightness is wonky with both versions (setting?)
// TODO: Understand the init sequence
// TODO: Understand the write-read process
// 	I think it works because after writing a request to the screen the SPI clock stops until you
// 	begin reading
