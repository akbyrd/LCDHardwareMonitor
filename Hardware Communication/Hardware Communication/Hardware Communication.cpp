#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstdint>

using u8 = unsigned char;
using u16 = unsigned short;

#define Assert(condition) if (!(condition)) { *((int*) 0) = 0; }
#define BYTE(n, val) u8(((val) >> ((n)*8)) & 0xFF)

#define ENABLE_TRACE false
#if ENABLE_TRACE
	#define Trace(...) printf(__VA_ARGS__)
#else
	#define Trace(...)
#endif

template<typename T, size_t S>
constexpr inline size_t ArrayLength(const T(&)[S]) { return S; }

template <typename T>
T Min(T a, T b) { return a <= b ? a : b; }

enum struct Signal
{
	Low = 0,
	High = 1,
};

#include "FT232H.h"
#include "ILI9341.h"

int main()
{
	FT232H::State ft232h = {};

	FT232H_Initialize(&ft232h);
	ILI9341_Initialize(&ft232h);

	ILI9341_Write(&ft232h, 0x2A, 0x00, 0xA0, 0x00, 0xA0); // Column
	ILI9341_Write(&ft232h, 0x2B, 0x00, 0x78, 0x00, 0x78); // Page
	ILI9341_Write(&ft232h, 0x2C, 0x00, 0x1F);             // Memory

	ILI9341_Finalize(&ft232h);
	FT232H_Finalize(&ft232h);
	return 0;
}

// Features for the final version
// 	Ability to trivially toggle tracing
// 	Ability to trivially toggle write batching
