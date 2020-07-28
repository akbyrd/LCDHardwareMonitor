#include "ftd2xx.h"
#pragma comment(lib, "ftd2xx.lib")

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

#define CheckStatus(status) \
	if (status != FT_OK) \
	{ \
		HandleError(status); \
		Assert(false); \
	}

namespace FT232H
{
	struct State
	{
		FT_HANDLE device            = nullptr;
		u8        latency           = 1;
		u8        lowPinValues      = 0b0000'0000; // Pin 2: DI, 1: DO, 0: TCK
		u8        lowPinDirections  = 0b0000'0011;
		u8        highPinValues     = 0b0000'0000; // Pin 2: RST, 1: D/C, 0: CS
		u8        highPinDirections = 0b0000'0011;
	};

	struct Command
	{
		static const u8 SendBytesFallingMSB  = '\x11';
		static const u8 RecvBytesRisingMSB   = '\x20';
		static const u8 RecvBytesFallingMSB   = '\x24';
		static const u8 SetDataBitsLowByte   = '\x80';
		static const u8 ReadDataBitsLowByte  = '\x81';
		static const u8 SetDataBitsHighByte  = '\x82';
		static const u8 ReadDataBitsHighByte = '\x83';
		static const u8 EnableLoopback       = '\x84';
		static const u8 DisableLoopback      = '\x85';
		static const u8 SetClockDivisor      = '\x86';
		static const u8 SendImmediate        = '\x87';
		static const u8 DisableClockDivide   = '\x8A';
		static const u8 EnableClockDivide    = '\x8B';
		static const u8 Enable3PhaseClock    = '\x8C';
		static const u8 Disable3PhaseClock   = '\x8D';
		static const u8 DisableAdaptiveClock = '\x97';
		static const u8 SetIODriveMode       = '\x9E';
		static const u8 BadCommand           = '\xAB';
	};

	struct Response
	{
		static const u8 BadCommand = '\xFA';
	};
}

// NOTE: Be *super* careful with sprinkling this around. It will trash performance in a hurry
void FT232H_WaitForResponse(FT232H::State* ft232h)
{
	Sleep(2 * ft232h->latency);
}

void FT232H_DrainRead(FT232H::State* ft232h, bool print)
{
	DWORD bytesInReadBuffer;
	FT_STATUS status = FT_GetQueueStatus(ft232h->device, &bytesInReadBuffer);
	CheckStatus(status);

	if (bytesInReadBuffer > 0)
	{
		if (print)
			Print("drain ");

		while (bytesInReadBuffer > 0)
		{
			u8 buffer[8];
			DWORD numBytesToRead = Min(bytesInReadBuffer, DWORD(ArrayLength(buffer)));

			DWORD numBytesRead = 0;
			status = FT_Read(ft232h->device, buffer, numBytesToRead, &numBytesRead);
			CheckStatus(status);
			Assert(numBytesToRead == numBytesRead);
			bytesInReadBuffer -= numBytesRead;

			if (print)
			{
				PrintBytesRaw(buffer);
				Print(" ");
			}
		}

		if (print)
			Print("\n");
	}
}

struct Command
{
	u8 command;
	u8 length;
	bool payload;
	const char* name;
};
Command commands[0xFF] = {};
Command* _commands = []()
{
	#define X2(x, ...) commands[x]={x, __VA_ARGS__}
	X2(0x00, 0, true,  "<UNKNOWN>             ");
	X2(0x10, 2, true,  "WRITE_BYTES_PVE_MSB   ");
	X2(0x11, 2, true,  "WRITE_BYTES_NVE_MSB   ");
	X2(0x12, 1, true,  "WRITE_BITS_PVE_MSB    ");
	X2(0x13, 1, true,  "WRITE_BITS_NVE_MSB    ");
	X2(0x18, 2, true,  "WRITE_BYTES_PVE_LSB   ");
	X2(0x19, 2, true,  "WRITE_BYTES_NVE_LSB   ");
	X2(0x1a, 1, true,  "WRITE_BITS_PVE_LSB    ");
	X2(0x1b, 1, true,  "WRITE_BITS_NVE_LSB    ");
	X2(0x20, 2, false, "READ_BYTES_PVE_MSB    ");
	X2(0x24, 2, false, "READ_BYTES_NVE_MSB    ");
	X2(0x22, 1, false, "READ_BITS_PVE_MSB     ");
	X2(0x26, 1, false, "READ_BITS_NVE_MSB     ");
	X2(0x28, 2, false, "READ_BYTES_PVE_LSB    ");
	X2(0x2c, 2, false, "READ_BYTES_NVE_LSB    ");
	X2(0x2a, 1, false, "READ_BITS_PVE_LSB     ");
	X2(0x2e, 1, false, "READ_BITS_NVE_LSB     ");
	X2(0x31, 2, true,  "RW_BYTES_PVE_NVE_MSB  ");
	X2(0x34, 2, true,  "RW_BYTES_NVE_PVE_MSB  ");
	X2(0x32, 1, true,  "RW_BITS_PVE_PVE_MSB (Invalid)");
	X2(0x33, 1, true,  "RW_BITS_PVE_NVE_MSB  ");
	X2(0x36, 1, true,  "RW_BITS_NVE_PVE_MSB  ");
	X2(0x37, 1, true,  "RW_BITS_NVE_NVE_MSB (Invalid)");
	X2(0x39, 2, true,  "RW_BYTES_PVE_NVE_LSB  ");
	X2(0x3c, 2, true,  "RW_BYTES_NVE_PVE_LSB  ");
	X2(0x3a, 1, true,  "RW_BITS_PVE_PVE_LSB (Invalid)");
	X2(0x3b, 1, true,  "RW_BITS_PVE_NVE_LSB   ");
	X2(0x3e, 1, true,  "RW_BITS_NVE_PVE_LSB   ");
	X2(0x3f, 1, true,  "RW_BITS_NVE_NVE_LSB (Invalid)");
	X2(0x4a, 1, true,  "WRITE_BITS_TMS_PVE    ");
	X2(0x4b, 1, true,  "WRITE_BITS_TMS_NVE    ");
	X2(0x6a, 1, true,  "RW_BITS_TMS_PVE_PVE   ");
	X2(0x6b, 1, true,  "RW_BITS_TMS_PVE_NVE   ");
	X2(0x6e, 1, true,  "RW_BITS_TMS_NVE_PVE   ");
	X2(0x6f, 1, true,  "RW_BITS_TMS_NVE_NVE   ");
	X2(0x87, 0, false, "SEND_IMMEDIATE        ");
	X2(0x88, 0, false, "WAIT_ON_HIGH          ");
	X2(0x89, 0, false, "WAIT_ON_LOW           ");
	X2(0x90, 1, false, "READ_SHORT            ");
	X2(0x91, 2, false, "READ_EXTENDED         ");
	X2(0x92, 2, false, "WRITE_SHORT           ");
	X2(0x93, 3, false, "WRITE_EXTENDED        ");
	X2(0x8a, 0, false, "DISABLE_CLK_DIV5      ");
	X2(0x8b, 0, false, "ENABLE_CLK_DIV5       ");
	X2(0x80, 2, false, "SET_BITS_LOW          ");
	X2(0x82, 2, false, "SET_BITS_HIGH         ");
	X2(0x81, 0, false, "GET_BITS_LOW          ");
	X2(0x83, 0, false, "GET_BITS_HIGH         ");
	X2(0x84, 0, false, "LOOPBACK_START        ");
	X2(0x85, 0, false, "LOOPBACK_END          ");
	X2(0x86, 2, false, "SET_TCK_DIVISOR       ");
	X2(0x8c, 0, false, "ENABLE_CLK_3PHASE     ");
	X2(0x8d, 0, false, "DISABLE_CLK_3PHASE    ");
	X2(0x8e, 1, false, "CLK_BITS_NO_DATA      ");
	X2(0x8f, 2, false, "CLK_BYTES_NO_DATA     ");
	X2(0x94, 0, false, "CLK_WAIT_ON_HIGH      ");
	X2(0x95, 0, false, "CLK_WAIT_ON_LOW       ");
	X2(0x96, 0, false, "ENABLE_CLK_ADAPTIVE   ");
	X2(0x97, 0, false, "DISABLE_CLK_ADAPTIVE  ");
	X2(0x9c, 2, false, "CLK_COUNT_WAIT_ON_HIGH");
	X2(0x9d, 2, false, "CLK_COUNT_WAIT_ON_LOW ");
	X2(0x9e, 2, false, "DRIVE_ZERO            ");
	X2(0xAB, 0, false, "BAD_COMMAND           ");
	return commands;
}();

void TraceDataWrite(u8* data, u16 dataLen)
{
#if TRACE_ENABLED
	Trace("fwrite data ");

	u8 consumed = 0;
	while (consumed < dataLen)
	{
		u8 cmd = data[consumed];
		Command command = commands[cmd];

		Trace("%s (0x%.2X)", command.name, cmd);
		consumed += 1;
		if (command.length > 0)
		{
			Print(" ");
			PrintBytesRaw(&data[consumed], command.length);
			consumed += command.length;
		}

		if (command.payload)
		{
			Trace(" - Payload");
			u8 payloadLength = dataLen - consumed;
			if (command.length == 2)
				payloadLength = data[consumed - 2] + 1;
			if (payloadLength > 0)
			{
				Trace(" ");
				TraceBytesRaw(&data[consumed], payloadLength);
				consumed += payloadLength;
			}
		}

		if (consumed < dataLen)
			Trace("\n            ");
	}
	Trace("\n");
#endif
}

void FT232H_Write(FT232H::State* ft232h, u8 command)
{
	//if (print)
		//PrintBytes("fwrite data", &command, 1);
		TraceDataWrite(&command, 1);

	DWORD bytesWritten;
	FT_STATUS status = FT_Write(ft232h->device, &command, 1, &bytesWritten);
	CheckStatus(status);
	Assert(bytesWritten == 1);

	//command = FT232H::Command::SendImmediate;
	//status = FT_Write(ft232h->device, &command, 1, &bytesWritten);
	//CheckStatus(status);
	//Assert(bytesWritten == 1);
};

void FT232H_Write(FT232H::State* ft232h, u8* data, u16 dataLen)
{
	//if (print)
		//PrintBytes("fwrite data", data, dataLen);
		TraceDataWrite(data, dataLen);

	DWORD bytesWritten;
	FT_STATUS status = FT_Write(ft232h->device, data, dataLen, &bytesWritten);
	CheckStatus(status);
	Assert(bytesWritten == dataLen);

	//u8 command = FT232H::Command::SendImmediate;
	//status = FT_Write(ft232h->device, &command, 1, &bytesWritten);
	//CheckStatus(status);
	//Assert(bytesWritten == 1);
};

template <u16 N>
void FT232H_Write(FT232H::State* ft232h, u8 (&data)[N])
{
	//if (print)
		//PrintBytes("fwrite data", data);
		TraceDataWrite(data, N);

	DWORD bytesWritten;
	FT_STATUS status = FT_Write(ft232h->device, data, N, &bytesWritten);
	CheckStatus(status);
	Assert(bytesWritten == N);

	//u8 command = FT232H::Command::SendImmediate;
	//status = FT_Write(ft232h->device, &command, 1, &bytesWritten);
	//CheckStatus(status);
	//Assert(bytesWritten == 1);
};

void FT232H_SetDC(FT232H::State* ft232h, Signal signal)
{
	u8 dcMask = 0b0000'0010;

	switch (signal)
	{
		default: Assert(false); break;

		case Signal::Low:
		{
			Trace("dc 0\n");
			ft232h->highPinValues &= ~dcMask;
			u8 pinInitCmd[] = { FT232H::Command::SetDataBitsHighByte, ft232h->highPinValues, ft232h->highPinDirections };
			FT232H_Write(ft232h, pinInitCmd);
			break;
		}

		case Signal::High:
		{
			Trace("dc 1\n");
			ft232h->highPinValues |= dcMask;
			u8 pinInitCmd[] = { FT232H::Command::SetDataBitsHighByte, ft232h->highPinValues, ft232h->highPinDirections };
			FT232H_Write(ft232h, pinInitCmd);
			break;
		}
	}
}

void FT232H_SetCS(FT232H::State* ft232h, Signal signal)
{
	u8 csMask = 0b0000'0001;

	switch (signal)
	{
		default: Assert(false); break;

		case Signal::Low:
		{
			Trace("cs 0\n");
			ft232h->highPinValues &= ~csMask;
			u8 pinInitCmd[] = { FT232H::Command::SetDataBitsHighByte, ft232h->highPinValues, ft232h->highPinDirections };
			FT232H_Write(ft232h, pinInitCmd);
			break;
		}

		case Signal::High:
		{
			Trace("cs 1\n");
			ft232h->highPinValues |= csMask;
			u8 pinInitCmd[] = { FT232H::Command::SetDataBitsHighByte, ft232h->highPinValues, ft232h->highPinDirections };
			FT232H_Write(ft232h, pinInitCmd);
			break;
		}
	}
}

template <u16 N>
u16 FT232H_Read(FT232H::State* ft232h, u8 (&buffer)[N])
{
	FT_STATUS status;

	// NOTE: ILI9341 shifts on the falling edge, therefore read on the rising edge
	DWORD numBytesWritten;
	u8 ftcmd[] = { FT232H::Command::RecvBytesRisingMSB, BYTE(0, N - 1), BYTE(1, N - 1) };
	//u8 ftcmd[] = { FT232H::Command::RecvBytesFallingMSB, BYTE(0, N - 1), BYTE(1, N - 1), FT232H::Command::SendImmediate };
	status = FT_Write(ft232h->device, ftcmd, ArrayLength(ftcmd), &numBytesWritten);
	CheckStatus(status);
	Assert(ArrayLength(ftcmd) == numBytesWritten);

	// TODO: We're waiting twice. Are both necessary?
	//FT232H_WaitForResponse(ft232h);

	// TODO: Don't need this if we use timeouts
	// NOTE: Check the queue size so we don't stall in the read
	//DWORD bytesInReadBuffer;
	//status = FT_GetQueueStatus(ft232h->device, &bytesInReadBuffer);
	//CheckStatus(status);

	//DWORD numBytesToRead = Min(bytesInReadBuffer, DWORD(N));
	DWORD numBytesToRead = N;

	DWORD numBytesRead = 0;
	status = FT_Read(ft232h->device, buffer, numBytesToRead, &numBytesRead);
	CheckStatus(status);
	Assert(numBytesToRead == numBytesRead);

	FT232H_SetCS(ft232h, Signal::High);
	FT232H_SetCS(ft232h, Signal::Low);

	TraceBytes("fread data", buffer);

	return u16(numBytesRead);
}

void FT232H_AssertEmptyReadBuffer(FT232H::State* ft232h)
{
	DWORD bytesInReadBuffer;
	FT_STATUS status = FT_GetQueueStatus(ft232h->device, &bytesInReadBuffer);
	CheckStatus(status);

	// DEBUG
	if (bytesInReadBuffer > 0)
	{
		Trace("AssertEmptyReadBuffer failed\n");
		FT232H_DrainRead(ft232h, true);
	}
	Assert(bytesInReadBuffer == 0);
}

void FT232H_Initialize(FT232H::State* ft232h)
{
	FT_HANDLE& device = ft232h->device;
	UCHAR& latency = ft232h->latency;

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

		PrintBytes("flush", buffer, (u16) numBytesRead);
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
	ft232h->latency = 1;
	status = FT_SetLatencyTimer(device, ft232h->latency);
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

	FT232H_Write(ft232h, FT232H::Command::EnableLoopback);
	// TODO: Do we need to wait here?
	// NOTE: Just following AN_135
	FT232H_AssertEmptyReadBuffer(ft232h);

	// NOTE: Sync MPSSE by writing a bogus command and reading the result
	FT232H_Write(ft232h, FT232H::Command::BadCommand);

	// TODO: Can we use FT232H_WaitForResponse?
	// NOTE: It takes a bit for the response to show up
	while (bytesInReadBuffer == 0)
	{
		status = FT_GetQueueStatus(device, &bytesInReadBuffer);
		CheckStatus(status);
	}

	// TODO: Seems like we kinda need a flush after enabling MPSSE
	// DEBUG
	if (bytesInReadBuffer != 2)
		FT232H_DrainRead(ft232h, true);
	Assert(bytesInReadBuffer == 2);

	u8 buffer[2];
	DWORD numBytesRead = 0;
	status = FT_Read(device, buffer, (u32) ArrayLength(buffer), &numBytesRead);
	CheckStatus(status);
	Assert(bytesInReadBuffer == numBytesRead);

	Assert(buffer[0] == FT232H::Response::BadCommand);
	Assert(buffer[1] == FT232H::Command::BadCommand);

	TraceBytes("fread data", buffer, (u16) numBytesRead);

	FT232H_Write(ft232h, FT232H::Command::EnableClockDivide);
	FT232H_Write(ft232h, FT232H::Command::DisableAdaptiveClock);
	FT232H_Write(ft232h, FT232H::Command::Enable3PhaseClock);
	//u8 mode[] = { 0x07, 0x00 };
	//FT232H_Write(ft232h, FT232H::Command::SetIODriveMode, mode);

	// TODO: If this is anything other than 0 reads are trash
	// NOTE: SCL Frequency = 60 Mhz / ((1 + divisor) * 2 * 5or1)
	u16 clockDivisor = 0x0000;
	u8 clockCmd[] = { FT232H::Command::SetClockDivisor, BYTE(0, clockDivisor), BYTE(1, clockDivisor) };
	FT232H_Write(ft232h, clockCmd);

	// Pin 2: DI, 1: DO, 0: TCK
	ft232h->lowPinValues     = 0b1111'1111; // 0xFF
	ft232h->lowPinDirections = 0b1111'1011; // 0xFB
	u8 pinInitLCmd[] = { FT232H::Command::SetDataBitsLowByte, ft232h->lowPinValues, ft232h->lowPinDirections };
	FT232H_Write(ft232h, pinInitLCmd);

	// Pin 2: RST, 1: D/C, 0: CS
	ft232h->highPinValues     = 0b1111'1110; // 0xFE
	ft232h->highPinDirections = 0b1111'1111; // 0xFF
	u8 pinInitHCmd[] = { FT232H::Command::SetDataBitsHighByte, ft232h->highPinValues, ft232h->highPinDirections };
	FT232H_Write(ft232h, pinInitHCmd);

	// TODO: Is this needed to set pin states? Why is it down here?
	FT232H_Write(ft232h, FT232H::Command::DisableLoopback);
	// NOTE: Just following AN_135
	// TODO: Do we need to wait here?
	FT232H_AssertEmptyReadBuffer(ft232h);
}

void FT232H_Finalize(FT232H::State* ft232h)
{
	FT_HANDLE& device = ft232h->device;
	UCHAR& latency = ft232h->latency;

	// NOTE: If we don't have this sleep here the device gets into a bad state. It seems either
	// resetting or closing the device while data is pending is a bad idea. We'll need to account for
	// this since it can happen in the wild if e.g. a user unplugs the device.
	FT232H_WaitForResponse(ft232h);
	FT232H_AssertEmptyReadBuffer(ft232h);

	// NOTE: Just following AN_135
	// Reset MPSSE controller
	UCHAR mask = 0;
	FT_STATUS status = FT_SetBitMode(device, mask, FT_BITMODE_RESET);
	CheckStatus(status);

	status = FT_Close(device);
	CheckStatus(status);
}
