#include "ftd2xx.h"

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
		UCHAR     latency           = 1;
		u8        lowPinValues      = 0b0000'0000; // Pin 2: DI, 1: DO, 0: TCK
		u8        lowPinDirections  = 0b0000'0011;
		u8        highPinValues     = 0b0000'0000; // Pin 2: RST, 1: D/C, 0: CS
		u8        highPinDirections = 0b0000'0011;
	};

	struct Command
	{
		static const u8 WRITE_BYTES_PVE_MSB  = '\x10';
		static const u8 WRITE_BYTES_NVE_MSB  = '\x11';
		static const u8 READ_BYTES_PVE_MSB   = '\x20';
		static const u8 READ_BYTES_NVE_MSB   = '\x24';
		static const u8 RW_BYTES_PVE_NVE_MSB = '\x31';
		static const u8 RW_BYTES_NVE_PVE_MSB = '\x34';

		//static const u8 SendBytesRisingMSBFirst     = '\x10';
		//static const u8 SendRecvBytesRisingMSBFirst = '\x34';
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
}

// NOTE: Be *super* careful with sprinkling this around. It will trash performance in a hurry
void FT232H_WaitForResponse(FT232H::State* ft232h)
{
	Sleep(2 * ft232h->latency);
}

void FT232H_DrainRead(FT232H::State* ft232h, bool force = false)
{
#if !ENABLE_TRACE || true
	if (!force)
		return;
#endif

	DWORD bytesInReadBuffer;
	FT_STATUS status = FT_GetQueueStatus(ft232h->device, &bytesInReadBuffer);
	CheckStatus(status);

	if (bytesInReadBuffer > 0)
	{
		Trace("drain");
		while (bytesInReadBuffer > 0)
		{
			u8 buffer[8];
			DWORD numBytesToRead = Min(bytesInReadBuffer, DWORD(ArrayLength(buffer)));

			DWORD numBytesRead = 0;
			status = FT_Read(ft232h->device, buffer, numBytesToRead, &numBytesRead);
			CheckStatus(status);
			Assert(numBytesToRead == numBytesRead);
			bytesInReadBuffer -= numBytesRead;

#if ENABLE_TRACE
			for (int i = 0; i < int(numBytesRead); i++)
				Trace(" 0x%.2X", buffer[i]);
#endif
		}
		Trace("\n");
	}
}

void FT232H_Write(FT232H::State* ft232h, u8 command, bool drain = true)
{
	DWORD bytesWritten;
	FT_STATUS status = FT_Write(ft232h->device, &command, 1, &bytesWritten);
	CheckStatus(status);
	Assert(bytesWritten == 1);

#if ENABLE_TRACE
	if (drain)
	{
		FT232H_WaitForResponse(ft232h);
		FT232H_DrainRead(ft232h);
	}
#endif
};

void FT232H_Write(FT232H::State* ft232h, u8* data, u16 dataLen, bool drain = true)
{
	DWORD bytesWritten;
	FT_STATUS status = FT_Write(ft232h->device, data, dataLen, &bytesWritten);
	CheckStatus(status);
	Assert(bytesWritten == dataLen);

#if ENABLE_TRACE
	if (drain)
	{
		FT232H_WaitForResponse(ft232h);
		FT232H_DrainRead(ft232h);
	}
#endif
};

template <u16 N>
void FT232H_Write(FT232H::State* ft232h, u8 (&data)[N], bool drain = true)
{
	DWORD bytesWritten;
	FT_STATUS status = FT_Write(ft232h->device, data, N, &bytesWritten);
	CheckStatus(status);
	Assert(bytesWritten == N);

#if ENABLE_TRACE
	if (drain)
	{
		FT232H_WaitForResponse(ft232h);
		FT232H_DrainRead(ft232h);
	}
#endif
};

void FT232H_SetDC(FT232H::State* ft232h, Signal signal)
{
	u8 dcMask = 0b0000'0010;

	switch (signal)
	{
		default: Assert(false);

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

#if true
template <u16 N>
u16 FT232H_Read(FT232H::State* ft232h, u8 (&buffer)[N])
{
	FT_STATUS status;

	// NOTE: ILI9341 shifts on the falling edge, therefore read on the rising edge
	DWORD numBytesWritten;
	u8 ftcmd[] = { FT232H::Command::READ_BYTES_PVE_MSB, BYTE(0, N - 1), BYTE(1, N - 1) };
	status = FT_Write(ft232h->device, ftcmd, ArrayLength(ftcmd), &numBytesWritten);
	CheckStatus(status);
	Assert(ArrayLength(ftcmd) == numBytesWritten);

	// TODO: We're wating twice. Are both necessary?
	//FT232H_WaitForResponse(ft232h);

	// TODO: Don't need this if we use timeouts
	// NOTE: Check the queue size so we don't stall in the read
	//DWORD bytesInReadBuffer;
	//status = FT_GetQueueStatus(ft232h->device, &bytesInReadBuffer);
	//CheckStatus(status);

	Trace("read data");
	//DWORD numBytesToRead = Min(bytesInReadBuffer, DWORD(N));
	DWORD numBytesToRead = N;

	DWORD numBytesRead = 0;
	status = FT_Read(ft232h->device, buffer, numBytesToRead, &numBytesRead);
	CheckStatus(status);
	Assert(numBytesToRead == numBytesRead);

#if ENABLE_TRACE
	for (int i = 0; i < int(numBytesRead); i++)
		Trace(" 0x%.2X", buffer[i]);
#endif
	Trace("\n");

	return u16(numBytesRead);
}
#else
template <u16 N>
u16 FT232H_Read(FT232H::State* ft232h, u8 ilicmd, u8 (&buffer)[N])
{
	FT232H_SetDC(ft232h, Signal::Low);

	u16 cmdSize = 0;
	u8 cmd[1024];

	cmd[cmdSize++] = FT232H::Command::WRITE_BYTES_NVE_MSB;
	cmd[cmdSize++] = BYTE(0, 1 - 1);
	cmd[cmdSize++] = BYTE(1, 1 - 1);
	cmd[cmdSize++] = ilicmd;
	cmd[cmdSize++] = FT232H::Command::READ_BYTES_PVE_MSB;
	cmd[cmdSize++] = BYTE(0, N - 1);
	cmd[cmdSize++] = BYTE(1, N - 1);

	FT_STATUS status;

	DWORD numBytesWritten;
	status = FT_Write(ft232h->device, cmd, cmdSize, &numBytesWritten);
	CheckStatus(status);
	Assert(cmdSize == numBytesWritten);

	FT232H_WaitForResponse(ft232h);

	DWORD numBytesRead = 0;
	status = FT_Read(ft232h->device, buffer, N, &numBytesRead);
	CheckStatus(status);
	Assert(N == numBytesRead);

	return u16(numBytesRead);
}
#endif

void FT232H_AssertEmptyReadBuffer(FT232H::State* ft232h)
{
	DWORD bytesInReadBuffer;
	FT_STATUS status = FT_GetQueueStatus(ft232h->device, &bytesInReadBuffer);
	CheckStatus(status);
	Assert(bytesInReadBuffer == 0);

	// DEBUG
	if (bytesInReadBuffer > 0)
	{
		Trace("AssertEmptyReadBuffer failed\n");
		FT232H_DrainRead(ft232h, true);
	}
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

	FT232H_Write(ft232h, FT232H::Command::EnableLoopback, false);
	// TODO: Do we need to wait here?
	// NOTE: Just following AN_135
	FT232H_AssertEmptyReadBuffer(ft232h);

	// NOTE: Sync MPSSE by writing a bogus command and reading the result
	FT232H_Write(ft232h, FT232H::Command::BadCommand, false);

	// TODO: Can we use FT232H_WaitForResponse?
	// NOTE: It takes a bit for the response to show up
	while (bytesInReadBuffer == 0)
	{
		status = FT_GetQueueStatus(device, &bytesInReadBuffer);
		CheckStatus(status);
	}
	Assert(bytesInReadBuffer == 2);

	// TODO: Seems like we kinda need a flush after enabling MPSSE
	// DEBUG
	if (bytesInReadBuffer != 2)
		FT232H_DrainRead(ft232h);

	u8 buffer[2];
	DWORD numBytesRead = 0;
	status = FT_Read(device, buffer, ArrayLength(buffer), &numBytesRead);
	CheckStatus(status);
	Assert(bytesInReadBuffer == numBytesRead);

	Assert(buffer[0] == FT232H::Response::BadCommand);
	Assert(buffer[1] == FT232H::Command::BadCommand);

	FT232H_Write(ft232h, FT232H::Command::EnableClockDivide);
	FT232H_Write(ft232h, FT232H::Command::DisableAdaptiveClock);
	FT232H_Write(ft232h, FT232H::Command::Disable3PhaseClock);

	// NOTE: SCL Frequency = 60 Mhz / ((1 + divisor) * 2 * 5or1)
	u16 clockDivisor = 0x0000;
	u8 clockCmd[] = { FT232H::Command::SetClockDivisor, BYTE(0, clockDivisor), BYTE(1, clockDivisor) };
	FT232H_Write(ft232h, clockCmd);

	// Pin 2: DI, 1: DO, 0: TCK
	//u8 pinValues = 0b1100'1001;
	ft232h->lowPinValues = 0b1100'0001;
	ft232h->lowPinDirections = 0b1111'1011;
	u8 pinInitLCmd[] = { FT232H::Command::SetDataBitsLowByte, ft232h->lowPinValues, ft232h->lowPinDirections };
	FT232H_Write(ft232h, pinInitLCmd);

	// Pin 2: RST, 1: D/C, 0: CS
	ft232h->highPinValues = 0b0000'0010;
	ft232h->highPinDirections = 0b0000'0011;
	u8 pinInitHCmd[] = { FT232H::Command::SetDataBitsHighByte, ft232h->highPinValues, ft232h->highPinDirections };
	FT232H_Write(ft232h, pinInitHCmd);

	FT232H_Write(ft232h, FT232H::Command::DisableLoopback, false);
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
