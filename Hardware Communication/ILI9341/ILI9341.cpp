#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cstdint>

#include "ftd2xx.h"

using u8 = unsigned char;
using u16 = unsigned short;

#define Assert(condition) if (!(condition)) { *((int*) 0) = 0; }
#define BYTE(n, val) u8(((val) >> ((n)*8)) & 0xFF)

template<typename T, size_t S>
constexpr inline size_t ArrayLength(const T(&)[S]) { return S; }

void HandleError(FT_STATUS status)
{
#define X(s) case s: OutputDebugStringA(#s "\n"); Assert(false); break;
	switch (status)
	{
		default:
			OutputDebugStringA("Unrecognized status\n");
			Assert(false);
			break;

			X(FT_INVALID_HANDLE);
			X(FT_DEVICE_NOT_FOUND);
			X(FT_DEVICE_NOT_OPENED);
			X(FT_IO_ERROR);
			X(FT_INSUFFICIENT_RESOURCES);
			X(FT_INVALID_PARAMETER);
			X(FT_INVALID_BAUD_RATE);
			X(FT_DEVICE_NOT_OPENED_FOR_ERASE);
			X(FT_DEVICE_NOT_OPENED_FOR_WRITE);
			X(FT_FAILED_TO_WRITE_DEVICE);
			X(FT_EEPROM_READ_FAILED);
			X(FT_EEPROM_WRITE_FAILED);
			X(FT_EEPROM_ERASE_FAILED);
			X(FT_EEPROM_NOT_PRESENT);
			X(FT_EEPROM_NOT_PROGRAMMED);
			X(FT_INVALID_ARGS);
			X(FT_NOT_SUPPORTED);
			X(FT_OTHER_ERROR);
			X(FT_DEVICE_LIST_NOT_READY);
	}
#undef x
}

template <typename T>
T Min(T a, T b) { return a <= b ? a : b; }

#define CheckStatus(status) \
	if (status != FT_OK) \
	{ \
		HandleError(status); \
		Assert(false); \
	}

#define Trace(msg) \
	OutputDebugStringA(msg);

struct Command
{
	static const u8 WRITE_BYTES_PVE_MSB  = '\x10';
	static const u8 WRITE_BYTES_NVE_MSB  = '\x11';
	static const u8 RW_BYTES_PVE_NVE_MSB = '\x31';
	static const u8 RW_BYTES_NVE_PVE_MSB = '\x34';

	static const u8 SendBytesRisingMSBFirst     = '\x10';
	static const u8 SendRecvBytesRisingMSBFirst = '\x34';
	static const u8 SetDataBitsLowByte          = '\x80';
	static const u8 ReadDataBitsLowByte         = '\x81';
	static const u8 SetDataBitsHighByte         = '\x82';
	static const u8 EnableLoopback              = '\x84';
	static const u8 DisableLoopback             = '\x85';
	static const u8 SetClockDivisor             = '\x86';
	static const u8 DisableClockDivide          = '\x8A';
	static const u8 EnableClockDivide           = '\x8B';
	static const u8 Enable3PhaseClock           = '\x8C';
	static const u8 Disable3PhaseClock          = '\x8D';
	static const u8 DisableAdaptiveClock        = '\x97';
	static const u8 BadCommand                  = '\xAB';
};

struct Response
{
	static const u8 BadCommand = '\xFA';
};

FT_STATUS Write(FT_HANDLE device, u8 command)
{
	DWORD bytesWritten;
	FT_STATUS status = FT_Write(device, &command, 1, &bytesWritten);
	CheckStatus(status);
	Assert(bytesWritten == 1);
	return status;
};

template <u16 N>
FT_STATUS Write(FT_HANDLE device, u8 (&data)[N])
{
	DWORD bytesWritten;
	FT_STATUS status = FT_Write(device, data, N, &bytesWritten);
	CheckStatus(status);
	Assert(bytesWritten == N);
	return status;
};

template <u16 N>
void WriteToScreen(FT_HANDLE device, u8 ilicmd, u8 (&ilidata)[N])
{
	// Set D/C to 0 for command
	// Send ilicmd to screen
	// Set D/C to 1 for data
	// Send ilidata to screen

	// Pin 2: RST, 1: D/C, 0: CS
	// D/C: 0 Command, 1 Data

	printf("dc 0\n");
	u8 pinValues = 0b0000'0000;
	u8 pinDirections = 0b0000'0011;
	u8 pinInitCmd[] = { Command::SetDataBitsHighByte, pinValues, pinDirections };
	Write(device, pinInitCmd);

	printf("cmd 0x%.2X\n", ilicmd);
	u16 ftcmdSize = 1;
	u8 ftcmd[] = { Command::WRITE_BYTES_NVE_MSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
	Write(device, ftcmd);
	Write(device, ilicmd);

	printf("dc 1\n");
	pinValues = 0b0000'0010;
	pinDirections = 0b0000'0011;
	u8 pinInitData[] = { Command::SetDataBitsHighByte, pinValues, pinDirections };
	Write(device, pinInitData);

	printf("data");
	for (int i = 0; i < N; i++) printf(" 0x%.2X", ilidata[i]);
	printf("\n");
	ftcmdSize = N;
	u8 ftcmd2[] = { Command::WRITE_BYTES_NVE_MSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
	Write(device, ftcmd2);
	Write(device, ilidata);
};

#include "Example.h"

void AssertEmptyReadBuffer(FT_HANDLE device)
{
	DWORD bytesInReadBuffer;
	FT_STATUS status = FT_GetQueueStatus(device, &bytesInReadBuffer);
	CheckStatus(status);
	Assert(bytesInReadBuffer == 0);

	// DEBUG
	while (bytesInReadBuffer > 0)
	{
		u8 buffer[8];
		DWORD numBytesToRead = Min(bytesInReadBuffer, DWORD(ArrayLength(buffer)));

		DWORD numBytesRead = 0;
		status = FT_Read(device, buffer, numBytesToRead, &numBytesRead);
		CheckStatus(status);
		Assert(numBytesToRead == numBytesRead);
		bytesInReadBuffer -= numBytesRead;
	}
}

int main()
{
	FT_STATUS status = -1;

	// NOTE: Device list only updates when calling this function
	DWORD deviceCount;
	status = FT_CreateDeviceInfoList(&deviceCount);
	CheckStatus(status);

	union DeviceFlags
	{
		struct
		{
			bool isOpen      : 1;
			bool isHighSpeed : 1;
		};
		ULONG raw;
	};
	static_assert(sizeof(DeviceFlags) == sizeof(FT_DEVICE_LIST_INFO_NODE::Flags), "");

	FT_DEVICE_LIST_INFO_NODE deviceInfos[8];
	Assert(ArrayLength(deviceInfos) >= deviceCount);

	DWORD deviceInfoCount;
	status = FT_GetDeviceInfoList(deviceInfos, &deviceInfoCount);
	CheckStatus(status);

	if (deviceInfoCount > 0)
	{
		DeviceFlags deviceFlags;
		deviceFlags.raw = deviceInfos[0].Flags;
	}

	FT_HANDLE device = nullptr;
	status = FT_Open(0, &device);
	CheckStatus(status);

	// NOTE: Just following AN_135
	status = FT_ResetDevice(device);
	CheckStatus(status);

	Sleep(50);

	// NOTE: Flush device by reading
	DWORD bytesInReadBuffer;
	status = FT_GetQueueStatus(device, &bytesInReadBuffer);
	CheckStatus(status);

	while (bytesInReadBuffer > 0)
	{
		u8 buffer[8];
		DWORD numBytesToRead = Min(bytesInReadBuffer, DWORD(ArrayLength(buffer)));

		DWORD numBytesRead = 0;
		status = FT_Read(device, buffer, numBytesToRead, &numBytesRead);
		CheckStatus(status);
		Assert(numBytesToRead == numBytesRead);
		bytesInReadBuffer -= numBytesRead;
	}

	// NOTE: This drops any data held in the driver
	// NOTE: Supposedly, only "in" actually works
	// NOTE: 64 B - 64 kB (4096 B default)
	DWORD inTransferSize  = 64 * 1024;
	DWORD outTransferSize = 64 * 1024;
	status = FT_SetUSBParameters(device, inTransferSize, outTransferSize);
	CheckStatus(status);

	// TODO: What are the defaults?
	bool eventEnabled = false;
	bool errorEnabled = false;
	status = FT_SetChars(device, 0, eventEnabled, 0, errorEnabled);
	CheckStatus(status);

	// TODO: Yuck, there's no way to know if a read timed out?
	//ULONG readTimeoutMs = FT_DEFAULT_RX_TIMEOUT;
	//ULONG writeTimeoutMs = FT_DEFAULT_TX_TIMEOUT;
	ULONG readTimeoutMs = 0;
	ULONG writeTimeoutMs = 0;
	status = FT_SetTimeouts(device, readTimeoutMs, writeTimeoutMs);
	CheckStatus(status);

	#define FTX_DEFAULT_LATENCY 16
	UCHAR latency = 1;
	status = FT_SetLatencyTimer(device, latency);
	CheckStatus(status);

	status = FT_SetFlowControl(device, FT_FLOW_RTS_CTS, 0, 0);
	CheckStatus(status);

	// Reset MPSSE controller
	UCHAR mask = 0;
	status = FT_SetBitMode(device, mask, FT_BITMODE_RESET);
	CheckStatus(status);

	mask = 0;
	status = FT_SetBitMode(device, mask, FT_BITMODE_MPSSE);
	CheckStatus(status);

	// NOTE: MPSSE is ready to accept commands

	Write(device, Command::EnableLoopback);
	// NOTE: Just following AN_135
	AssertEmptyReadBuffer(device);

	// NOTE: Sync MPSSE by writing a bogus command and reading the result
	Write(device, Command::BadCommand);

	// NOTE: It takes a bit for the response to show up
	while (bytesInReadBuffer == 0)
	{
		status = FT_GetQueueStatus(device, &bytesInReadBuffer);
		CheckStatus(status);
	}
	Assert(bytesInReadBuffer == 2);

	// TODO: Seems like we kinda need a flush after setting MPSSE
	// DEBUG
	if (bytesInReadBuffer != 2)
	{
		while (bytesInReadBuffer > 0)
		{
			u8 buffer[8];
			DWORD numBytesToRead = Min(bytesInReadBuffer, DWORD(ArrayLength(buffer)));

			DWORD numBytesRead = 0;
			status = FT_Read(device, buffer, numBytesToRead, &numBytesRead);
			CheckStatus(status);
			Assert(numBytesToRead == numBytesRead);
			bytesInReadBuffer -= numBytesRead;
		}
	}

	u8 buffer[2];
	DWORD numBytesRead = 0;
	status = FT_Read(device, buffer, ArrayLength(buffer), &numBytesRead);
	CheckStatus(status);
	Assert(bytesInReadBuffer == numBytesRead);

	Assert(buffer[0] == Response::BadCommand);
	Assert(buffer[1] == Command::BadCommand);

	//Write(device, Command::DisableClockDivide);
	Write(device, Command::EnableClockDivide);
	Write(device, Command::DisableAdaptiveClock);
	Write(device, Command::Disable3PhaseClock);
	//Write(device, Command::Enable3PhaseClock);

	// SCL Frequency = 60 Mhz / ((1 + 0x05DB) * 2) = 1 Mhz
	//u16 clockDivisor = 0x05DB;
	u16 clockDivisor = 0x0000;
	u8 clockCmd[] = { Command::SetClockDivisor, BYTE(0, clockDivisor), BYTE(1, clockDivisor) };
	Write(device, clockCmd);

	// Pin 2: DI, 1: DO, 0: TCK
	//u8 pinValues = 0b1100'1001;
	u8 pinValues = 0b1100'0001;
	u8 pinDirections = 0b1111'1011;
	u8 pinInitLCmd[] = { Command::SetDataBitsLowByte, pinValues, pinDirections };
	Write(device, pinInitLCmd);

	// Pin 2: RST, 1: D/C, 0: CS
	pinValues = 0b0000'0010;
	pinDirections = 0b0000'0011;
	u8 pinInitHCmd[] = { Command::SetDataBitsHighByte, pinValues, pinDirections };
	Write(device, pinInitHCmd);

	Write(device, Command::DisableLoopback);
	// NOTE: Just following AN_135
	AssertEmptyReadBuffer(device);

	//RunExample(device);
	PythonInitCopy(device);
	u8 columnCmd[] = { 0x00, 0xA0, 0x00, 0xA0 };
	u8 pageCmd[]   = { 0x00, 0x78, 0x00, 0x78 };
	u8 memoryCmd[] = { 0x00, 0x1F };
	WriteToScreen(device, 0x2A, columnCmd);
	WriteToScreen(device, 0x2B, pageCmd);
	WriteToScreen(device, 0x2C, memoryCmd);

	// NOTE: If we don't have this sleep here the device gets into a bad state. It seems either
	// resetting or closing the device while data is pending is a bad idea. We'll need to account for
	// this since it can happen in the wild if e.g. a user unplugs the device.
	Sleep(2 * latency); AssertEmptyReadBuffer(device);

	// NOTE: Just following AN_135
	// Reset MPSSE controller
	mask = 0;
	status = FT_SetBitMode(device, mask, FT_BITMODE_RESET);
	CheckStatus(status);

	status = FT_Close(device);
	CheckStatus(status);

	Trace("Success!");
	return 0;
}

// MCU 8080-II
// CSX is active low, enables the ILI9341
// RESX is active low, external reset signal
// WRX is parallel data write strobe
// RDX is parallel data read strobe
// Latches input on rising edge of WRX
// D[17:0] is parallel data
// Write cycle: WRX high -> low -> high
// Read cycle: RDX high -> low -> high

// SPI
// D[17:0] must be connected to ground (unused)
// 3 line mode embeds a D/CX bit in the packet
// MSB first
// "The serial interface is initialized when CSX is high"
// Write Flow (to ILI):
// 	1. Host set CSX low + set D/CX (D/CX first?)
// 	2. Rising SCL: ILI read D/CX
// 	3. Falling SCL: Host set D7
// 	4. Rising SCL: ILI read D7
// 	5. Falling SCL: Host set D6
// 	6. Rising SCL: ILI read D6
// Looks like SCL and SDA must be low while CSX is high
// 	Wait, docs day "invalid". I think that just means they're ignored
// "ILI latches the SDA (input data) at the rising edgoes of SCL"
// "then shifts SDA (output data) at falling edges of SCL"
// "After read status command sent SDA must be set tri-state before falling SCL of last bit"
// Need 10 us break after RESX
// CSX stop in the middle of command/data: partial byte is discarded
// Break in multiple parameters: accept full bytes, discard partial
// Pauses between parameters are fine
// 2 data transfer modes: 16 and 18 bpp

// Configure 232 to match ILI requirements
// 	[x] Select SPI mode (soldered)
// 	[x] Write on rising or falling?
// 	[ ] Set clock frequency (what's the valid range?)
// 	[ ] Start clock high or low?
// 	[ ]
// Read something from the display
// 	Identification information
// 	Status
// 	Power mode
// 	MADCTL
// 	Pixel format
// 	Image format
// 	Signal mode
// 	Self diagnostic
