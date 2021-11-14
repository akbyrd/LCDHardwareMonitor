#include "ftd2xx.h"
#pragma comment(lib, "ftd2xx.lib")

struct FT232HState
{
	FT_HANDLE device;
	b8        errorMode;
	b8        enableTracing;
	b8        enableDebugChecks;
	b8        inSPITransaction;
	b8        deferTransaction;
	u8        latency;
	u32       writeTimeout;
	u32       readTimeout;
	u32       clockSpeedOverride;

	u32       clockSpeedDesired;
	u32       clockSpeedActual;
	u8        lowPinValues;
	u8        lowPinDirections;
	u8        highPinValues;
	u8        highPinDirections;
	Bytes     pendingCommands;
};

namespace FT232H
{
	static const u32   SerialNumber  = 0x04036014;
	static const char* Description   = "FT232H";
	static const u32   ClockSpeedMax = 30'000'000;
	static const u32   ClockSpeedMin = u32(457.763);

	struct LowPins
	{
		static const u8 CLKBit = 0;
		static const u8 DOBit  = 1;
		static const u8 DIBit  = 2;

		static const u8 CLKMask = 1 << CLKBit;
		static const u8 DOMask  = 1 << DOBit;
		static const u8 DIMask  = 1 << DIBit;
	};

	struct HighPins
	{
		static const u8 CSBit  = 0;
		static const u8 DCBit  = 1;
		static const u8 RSTBit = 2;

		static const u8 CSMask  = 1 << CSBit;
		static const u8 DCMask  = 1 << DCBit;
		static const u8 RSTMask = 1 << RSTBit;
	};

	struct Command
	{
		static const u8 SendBytesFallingMSB  = 0x11;
		static const u8 RecvBytesRisingMSB   = 0x20;
		static const u8 SetDataBitsLowByte   = 0x80;
		static const u8 ReadDataBitsLowByte  = 0x81;
		static const u8 SetDataBitsHighByte  = 0x82;
		static const u8 ReadDataBitsHighByte = 0x83;
		static const u8 EnableLoopback       = 0x84;
		static const u8 DisableLoopback      = 0x85;
		static const u8 SetClockDivisor      = 0x86;
		static const u8 SendImmediate        = 0x87;
		static const u8 DisableClockDivide   = 0x8A;
		static const u8 EnableClockDivide    = 0x8B;
		static const u8 Enable3PhaseClock    = 0x8C;
		static const u8 Disable3PhaseClock   = 0x8D;
		static const u8 DisableAdaptiveClock = 0x97;
		static const u8 BadCommand           = 0xAB;
	};

	struct Response
	{
		static const u8 BadCommand = 0xFA;
	};
}

// -------------------------------------------------------------------------------------------------
// Internal functions

static void
EnterErrorMode(FT232HState& ft232h)
{
	ft232h.errorMode = true;
	List_Clear(ft232h.pendingCommands);
}

// NOTE: Be *super* careful with sprinkling this around. It will trash performance in a hurry
static void
WaitForResponse(FT232HState& ft232h)
{
	Sleep((DWORD) (2 * ft232h.latency));
}

static void
DrainRead(FT232HState& ft232h, StringView prefix)
{
	Assert(ft232h.pendingCommands.length == 0);
	if (ft232h.errorMode) return;

	u32 bytesInReadBuffer;
	FT_STATUS status = FT_GetQueueStatus(ft232h.device, (DWORD*) &bytesInReadBuffer);
	LOG_IF(status != FT_OK, EnterErrorMode(ft232h); return,
		Severity::Error, "Failed to get queue status: %", status);

	if (bytesInReadBuffer > 0)
	{
		Bytes bytes = {};
		defer { List_Free(bytes); };
		List_Reserve(bytes, bytesInReadBuffer);

		status = FT_Read(ft232h.device, bytes.data, bytesInReadBuffer, (DWORD*) &bytes.length);
		LOG_IF(status != FT_OK, EnterErrorMode(ft232h); return,
			Severity::Error, "Failed to flush read buffer: %", status);
		Assert(bytes.length == bytesInReadBuffer);

		if (ft232h.enableTracing)
			Bytes_Print(prefix, bytes);
	}
}

static void
AssertEmptyReadBuffer(FT232HState& ft232h)
{
	Assert(ft232h.pendingCommands.length == 0);
	if (ft232h.errorMode) return;

	u32 bytesInReadBuffer;
	FT_STATUS status = FT_GetQueueStatus(ft232h.device, (DWORD*) &bytesInReadBuffer);
	LOG_IF(status != FT_OK, EnterErrorMode(ft232h); return,
		Severity::Error, "Failed to get queue status: %", status);

	if (bytesInReadBuffer > 0)
	{
		bool enableTracing = ft232h.enableTracing;
		ft232h.enableTracing = true;
		DrainRead(ft232h, "assert empty read");
		ft232h.enableTracing = enableTracing;

		Assert(false);
	}
}

u32
ToString(String& string, FT_STATUS status)
{
	#define X(s) case s: return ToString(string, #s); break;
	switch (status)
	{
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

	Assert(false);
	return (u32) -1;
}

// -------------------------------------------------------------------------------------------------
// Public API Implementation - Core Functions

void
FT232H_SetTracing(FT232HState& ft232h, b8 enable)
{
	ft232h.enableTracing = enable;
}

// NOTE: Slow! This will enable waits after every write to ensure the read buffer is empty.
void
FT232H_SetDebugChecks(FT232HState& ft232h, b8 enable)
{
	ft232h.enableDebugChecks = enable;
}

void
FT232H_SetClockOverride(FT232HState& ft232h, b8 enable, u32 hz)
{
	Assert(!ft232h.inSPITransaction);
	Assert(hz >= FT232H::ClockSpeedMin);
	Assert(hz <= FT232H::ClockSpeedMax);
	ft232h.clockSpeedOverride = enable ? hz : 0;
}

b8
FT232H_HasError(FT232HState& ft232h)
{
	return ft232h.errorMode;
}

void
FT232H_Write(FT232HState& ft232h, ByteSlice bytes)
{
	if (ft232h.errorMode) return;

	if (ft232h.deferTransaction)
	{
		if (bytes.length <= ft232h.pendingCommands.capacity - ft232h.pendingCommands.length)
		{
			List_AppendRange(ft232h.pendingCommands, bytes);

			if (ft232h.enableTracing)
				Bytes_Print("queue", bytes);
		}
		else if (bytes.length < ft232h.pendingCommands.capacity)
		{
			FT232H_Flush(ft232h);
			List_AppendRange(ft232h.pendingCommands, bytes);

			if (ft232h.enableTracing)
				Bytes_Print("queue", bytes);
		}
		else
		{
			FT232H_Flush(ft232h);
			FT232H_WriteImmediate(ft232h, bytes);
		}
	}
	else
	{
		FT232H_WriteImmediate(ft232h, bytes);
	}
}

void
FT232H_WriteImmediate(FT232HState& ft232h, ByteSlice bytes)
{
	Assert(!Slice_IsSparse(bytes));
	Assert(bytes.length != 0);
	Assert(ft232h.pendingCommands.length == 0);
	if (ft232h.errorMode) return;

	u32 bytesWritten;
	FT_STATUS status = FT_Write(ft232h.device, bytes.data, bytes.length, (DWORD*) &bytesWritten);
	LOG_IF(status != FT_OK, EnterErrorMode(ft232h); return,
		Severity::Error, "Failed to write to device: %", status);
	Assert(bytesWritten == bytes.length);

	if (ft232h.enableTracing)
		Bytes_Print("write", bytes);
}

void
FT232H_Flush(FT232HState& ft232h)
{
	if (ft232h.errorMode) return;
	if (ft232h.pendingCommands.length == 0) return;

	if (ft232h.enableTracing)
		Platform_Print("flush\n");

	Bytes bytes = ft232h.pendingCommands;
	ft232h.pendingCommands.length = 0;
	FT232H_WriteImmediate(ft232h, bytes);
	List_Clear(bytes);
}

void
FT232H_Read(FT232HState& ft232h, Bytes& bytes, u16 numBytesToRead)
{
	Assert(numBytesToRead != 0);
	Assert(ft232h.pendingCommands.length == 0);
	if (ft232h.errorMode) return;

	FT_STATUS status;

	if (ft232h.enableDebugChecks)
	{
		u32 bytesInReadBuffer;
		status = FT_GetQueueStatus(ft232h.device, (DWORD*) &bytesInReadBuffer);
		LOG_IF(status != FT_OK, EnterErrorMode(ft232h); return,
			Severity::Error, "Failed to get queue status: %", status);

		if (bytesInReadBuffer < numBytesToRead)
		{
			// TODO: Could be smarter about this if tracking "last read time"
			// NOTE: The read below will block until the data arrives. Since getting the queue status
			// doesn't we need to wait to ensure the data has a chance to arrive.
			WaitForResponse(ft232h);

			status = FT_GetQueueStatus(ft232h.device, (DWORD*) &bytesInReadBuffer);
			LOG_IF(status != FT_OK, EnterErrorMode(ft232h); return,
				Severity::Error, "Failed to get queue status: %", status);
			Assert(bytesInReadBuffer >= numBytesToRead);
		}
	}

	List_Reserve(bytes, bytes.length + numBytesToRead);

	u32 numBytesRead;
	status = FT_Read(ft232h.device, &bytes.data[bytes.length], numBytesToRead, (DWORD*) &numBytesRead);
	LOG_IF(status != FT_OK, EnterErrorMode(ft232h); return,
		Severity::Error, "Failed to read from device: %", status);
	Assert(numBytesRead == numBytesToRead);
	bytes.length += numBytesRead;

	if (ft232h.enableTracing)
		Bytes_Print("read", List_Slice(bytes, bytes.length - numBytesToRead));
}

void
FT232H_SendBytes(FT232HState& ft232h, ByteSlice bytes)
{
	Assert(!Slice_IsSparse(bytes));
	Assert(bytes.length != 0);
	if (ft232h.errorMode) return;

	u32 remainingLen = bytes.length;
	while (remainingLen > 0)
	{
		ByteSlice chunk = {};
		chunk.data   = &bytes.data[bytes.length - remainingLen];
		chunk.length = Min(remainingLen, FT232H::MaxSendBytes);
		chunk.stride = 1;

		// NOTE: LCD reads on the rising edge so write on the falling edge
		u16 numBytesEnc = (u16) (chunk.length - 1);
		u8 ftcmd[] = { FT232H::Command::SendBytesFallingMSB, UnpackLSB2(numBytesEnc) };
		FT232H_Write(ft232h, ftcmd);
		FT232H_Write(ft232h, chunk);
		remainingLen -= chunk.length;
	}
}

void
FT232H_RecvBytes(FT232HState& ft232h, Bytes& bytes, u16 numBytesToRead)
{
	Assert(numBytesToRead != 0);
	if (ft232h.errorMode) return;

	// NOTE: LCD writes on the falling edge so read on the rising edge
	u16 numBytesEnc = (u16) (numBytesToRead - 1);
	u8 ftcmd[] = { FT232H::Command::RecvBytesRisingMSB, UnpackLSB2(numBytesEnc) };

	FT232H_Write(ft232h, ftcmd);
	FT232H_Flush(ft232h);
	FT232H_Read(ft232h, bytes, numBytesToRead);
}

void
FT232H_SetCLK(FT232HState& ft232h, Signal signal)
{
	if (ft232h.errorMode) return;

	if (ft232h.enableTracing)
		Bytes_Print("clk", (u8) signal);

	Assert(signal == Signal::Low || signal == Signal::High);
	u8 newValues = SetBit(ft232h.lowPinValues, FT232H::LowPins::CLKBit, (u8) signal);

	if (ft232h.lowPinValues != newValues)
	{
		ft232h.lowPinValues = newValues;
		u8 pinInitCmd[] = { FT232H::Command::SetDataBitsLowByte, ft232h.lowPinValues, ft232h.lowPinDirections };
		FT232H_Write(ft232h, pinInitCmd);
	}
}

void
FT232H_SetDO(FT232HState& ft232h, Signal signal)
{
	if (ft232h.errorMode) return;

	if (ft232h.enableTracing)
		Bytes_Print("do", (u8) signal);

	Assert(signal == Signal::Low || signal == Signal::High);
	u8 newValues = SetBit(ft232h.lowPinValues, FT232H::LowPins::DOBit, (u8) signal);

	if (ft232h.lowPinValues != newValues)
	{
		ft232h.lowPinValues = newValues;
		u8 pinInitCmd[] = { FT232H::Command::SetDataBitsLowByte, ft232h.lowPinValues, ft232h.lowPinDirections };
		FT232H_Write(ft232h, pinInitCmd);
	}
}

void
FT232H_SetCS(FT232HState& ft232h, Signal signal)
{
	if (ft232h.errorMode) return;

	if (ft232h.enableTracing)
		Bytes_Print("cs", (u8) signal);

	Assert(signal == Signal::Low || signal == Signal::High);
	u8 newValues = SetBit(ft232h.highPinValues, FT232H::HighPins::CSBit, (u8) signal);

	if (ft232h.highPinValues != newValues)
	{
		ft232h.highPinValues = newValues;
		u8 pinInitCmd[] = { FT232H::Command::SetDataBitsHighByte, ft232h.highPinValues, ft232h.highPinDirections };
		FT232H_Write(ft232h, pinInitCmd);
	}
}

void
FT232H_SetDC(FT232HState& ft232h, Signal signal)
{
	if (ft232h.errorMode) return;

	if (ft232h.enableTracing)
		Bytes_Print("dc", (u8) signal);

	Assert(signal == Signal::Low || signal == Signal::High);
	u8 newValues = SetBit(ft232h.highPinValues, FT232H::HighPins::DCBit, (u8) signal);

	if (ft232h.highPinValues != newValues)
	{
		ft232h.highPinValues = newValues;
		u8 pinInitCmd[] = { FT232H::Command::SetDataBitsHighByte, ft232h.highPinValues, ft232h.highPinDirections };
		FT232H_Write(ft232h, pinInitCmd);
	}
}

Signal
FT232H_GetCLK(FT232HState& ft232h)
{
	return (Signal) GetBit(ft232h.lowPinValues, FT232H::LowPins::CLKBit);
}

Signal
FT232H_GetDO(FT232HState& ft232h)
{
	return (Signal) GetBit(ft232h.lowPinValues, FT232H::LowPins::DOBit);
}

Signal
FT232H_GetCS(FT232HState& ft232h)
{
	return (Signal) GetBit(ft232h.highPinValues, FT232H::HighPins::CSBit);
}

Signal
FT232H_GetDC(FT232HState& ft232h)
{
	return (Signal) GetBit(ft232h.highPinValues, FT232H::HighPins::DCBit);
}

void
FT232H_BeginSPI(FT232HState& ft232h)
{
	if (ft232h.enableTracing)
		Platform_Print("begin spi\n");

	// NOTE: Currently assumes polarity 0
	Assert(!ft232h.inSPITransaction);
	ft232h.inSPITransaction = true;
	FT232H_SetCS(ft232h, Signal::Low);
}

void
FT232H_EndSPI(FT232HState& ft232h)
{
	// NOTE: Currently assumes polarity 0
	Assert(ft232h.inSPITransaction);
	FT232H_SetCS(ft232h, Signal::High);
	FT232H_SetDO(ft232h, Signal::Low);
	FT232H_SetCLK(ft232h, Signal::Low);
	ft232h.inSPITransaction = false;

	if (ft232h.enableTracing)
		Platform_Print("end spi\n");
}

void
FT232H_BeginSPIDeferred(FT232HState& ft232h)
{
	if (ft232h.enableTracing)
		Platform_Print("begin spi deferred\n");

	// NOTE: Currently assumes polarity 0
	Assert(!ft232h.inSPITransaction);
	ft232h.inSPITransaction = true;
	ft232h.deferTransaction = true;
	FT232H_SetCS(ft232h, Signal::Low);
}

void
FT232H_EndSPIDeferred(FT232HState& ft232h)
{
	// NOTE: Currently assumes polarity 0
	Assert(ft232h.inSPITransaction);
	FT232H_SetCS(ft232h, Signal::High);
	FT232H_SetDO(ft232h, Signal::Low);
	FT232H_SetCLK(ft232h, Signal::Low);
	FT232H_Flush(ft232h);
	ft232h.inSPITransaction = false;
	ft232h.deferTransaction = false;

	if (ft232h.enableTracing)
		Platform_Print("end spi deferred\n");
}

// TODO: Maybe speed should be a double?
u32
FT232H_SetClockSpeed(FT232HState& ft232h, u32 hz)
{
	Assert(hz >= FT232H::ClockSpeedMin);
	Assert(hz <= FT232H::ClockSpeedMax);

	// TODO: Move inside state check
	if (ft232h.enableTracing)
		Platform_Print("set clock\n");

	if (ft232h.clockSpeedOverride)
		hz = ft232h.clockSpeedOverride;

	if (ft232h.clockSpeedDesired != hz)
	{
		ft232h.clockSpeedDesired = hz;

		// NOTE: clock = 60 Mhz / ((1 + divisor) * 2 * 5or1)
		// NOTE: => divisor = (60 MHz / (2 * clock * 5or1)) - 1
		// NOTE: Assumes we aren't using the clock divide by five
		u16 clockDivisor = (u16) Round(Min(60'000'000.0 / (2 * hz) - 1, (r64) u16Max));
		ft232h.clockSpeedActual = (u32) Round(60'000'000.0 / ((1 + clockDivisor) * 2));

		u8 clockCmd[] = { FT232H::Command::SetClockDivisor, UnpackLSB2(clockDivisor) };
		FT232H_Write(ft232h, clockCmd);
	}

	return ft232h.clockSpeedActual;
}

u32
FT232H_GetClockSpeed(FT232HState& ft232h)
{
	return ft232h.clockSpeedDesired;
}

// -------------------------------------------------------------------------------------------------
// Public API Implementation - Core Functions

b8
FT232H_Initialize(FT232HState& ft232h)
{
	FT_STATUS status;

	List_Reserve(ft232h.pendingCommands, FT232H::MaxSendBytes);

	// NOTE: Device list only updates when calling this function
	u32 deviceCount;
	status = FT_CreateDeviceInfoList((DWORD*) &deviceCount);
	LOG_IF(status != FT_OK, return false,
		Severity::Warning, "Failed to create device info list: %", status);

	List<FT_DEVICE_LIST_INFO_NODE> deviceInfos = {};
	List_Reserve(deviceInfos, deviceCount);

	status = FT_GetDeviceInfoList(deviceInfos.data, (DWORD*) &deviceInfos.length);
	LOG_IF(status != FT_OK, return false,
		Severity::Warning, "Failed to get device info list: %", status);
	Assert(deviceInfos.length == deviceCount);

	// Select a device
	for (u32 i = 0; i < deviceInfos.length; i++)
	{
		FT_DEVICE_LIST_INFO_NODE& deviceInfo = deviceInfos[i];

		b8 isOpen      = (deviceInfo.Flags & FT_FLAGS_OPENED)  == FT_FLAGS_OPENED;
		b8 isHighSpeed = (deviceInfo.Flags & FT_FLAGS_HISPEED) == FT_FLAGS_HISPEED;

		// TODO: Set a custom serial number and open by that
		if (strcmp(deviceInfo.Description, FT232H::Description) != 0) continue;

		if (isOpen) continue;

		status = FT_Open(0, &ft232h.device);
		LOG_IF(status != FT_OK, continue,
			Severity::Warning, "Failed to open device: %", status);

		LOG_IF(!isHighSpeed, IGNORE,
			Severity::Warning, "Selected device is not high speed. Plugged into a slow USB port?");
	}

	LOG_IF(!ft232h.device, return false,
		Severity::Info, "Failed to find a device");

	// NOTE: Just following AN_135
	status = FT_ResetDevice(ft232h.device);
	LOG_IF(status != FT_OK, return false,
		Severity::Error, "Failed to reset device: %", status);

	// NOTE: This drops any data held in the driver
	// NOTE: Supposedly, only "in" actually works
	// NOTE: 64 B - 64 kB (4096 B default)
	u32 inTransferSize  = 64 * Kilobyte;
	u32 outTransferSize = 64 * Kilobyte;
	status = FT_SetUSBParameters(ft232h.device, inTransferSize, outTransferSize);
	LOG_IF(status != FT_OK, return false,
		Severity::Error, "Failed to set USB parameters: %", status);

	// TODO: What are the defaults?
	b8 eventEnabled = false;
	b8 errorEnabled = false;
	status = FT_SetChars(ft232h.device, 0, eventEnabled, 0, errorEnabled);
	LOG_IF(status != FT_OK, return false,
		Severity::Error, "Failed to set event and error characters: %", status);

	// TODO: Yuck, there's no way to know if a read timed out?
	ft232h.readTimeout = 0; // FT_DEFAULT_RX_TIMEOUT
	ft232h.writeTimeout = 0; // FT_DEFAULT_TX_TIMEOUT
	status = FT_SetTimeouts(ft232h.device, ft232h.readTimeout, ft232h.writeTimeout);
	LOG_IF(status != FT_OK, return false,
		Severity::Error, "Failed to set timeouts: %", status);

	#define FTX_DEFAULT_LATENCY 16
	ft232h.latency = 1;
	status = FT_SetLatencyTimer(ft232h.device, ft232h.latency);
	LOG_IF(status != FT_OK, return false,
		Severity::Error, "Failed to set latency: %", status);

	status = FT_SetFlowControl(ft232h.device, FT_FLOW_RTS_CTS, 0, 0);
	LOG_IF(status != FT_OK, return false,
		Severity::Error, "Failed to set flow control: %", status);

	// Reset the MPSSE controller
	u8 mask = 0;
	status = FT_SetBitMode(ft232h.device, mask, FT_BITMODE_RESET);
	LOG_IF(status != FT_OK, return false,
		Severity::Error, "Failed to reset MPSSE: %", status);

	// Enable the MPSSE controller
	status = FT_SetBitMode(ft232h.device, mask, FT_BITMODE_MPSSE);
	LOG_IF(status != FT_OK, return false,
		Severity::Error, "Failed to enable MPSSE: %", status);

	// TODO: Is it worth deferring the init process so we don't block?
	Sleep(50);

	// NOTE: MPSSE is ready to accept commands

	// NOTE: Sync MPSSE by writing a bogus command and reading the result
	{
		// NOTE: Flush device read buffer
		DrainRead(ft232h, "initial drain");

		FT232H_Write(ft232h, FT232H::Command::EnableLoopback);
		FT232H_Write(ft232h, FT232H::Command::BadCommand);
		WaitForResponse(ft232h);

		Bytes bytes = {};
		defer { List_Free(bytes); };

		FT232H_Read(ft232h, bytes, 2);
		Assert(bytes[0] == FT232H::Response::BadCommand);
		Assert(bytes[1] == FT232H::Command::BadCommand);

		FT232H_Write(ft232h, FT232H::Command::DisableLoopback);
	}

	FT232H_Write(ft232h, FT232H::Command::DisableClockDivide);
	FT232H_Write(ft232h, FT232H::Command::DisableAdaptiveClock);
	FT232H_Write(ft232h, FT232H::Command::Disable3PhaseClock);
	FT232H_SetClockSpeed(ft232h, FT232H::ClockSpeedMax);

	// Pin 2: DI, DO, CLK
	ft232h.lowPinValues     = 0b0000'0000;
	ft232h.lowPinDirections = 0b0000'0011;
	u8 pinInitLCmd[] = { FT232H::Command::SetDataBitsLowByte, ft232h.lowPinValues, ft232h.lowPinDirections };
	FT232H_Write(ft232h, pinInitLCmd);

	// Pin 2: RST, D/C, CS
	ft232h.highPinValues     = 0b0000'0011;
	ft232h.highPinDirections = 0b0000'0011;
	u8 pinInitHCmd[] = { FT232H::Command::SetDataBitsHighByte, ft232h.highPinValues, ft232h.highPinDirections };
	FT232H_Write(ft232h, pinInitHCmd);

	if (ft232h.enableDebugChecks)
	{
		WaitForResponse(ft232h);
		AssertEmptyReadBuffer(ft232h);
	}

	if (ft232h.errorMode) return false;
	return true;
}

void
FT232H_Teardown(FT232HState& ft232h)
{
	if (!ft232h.errorMode)
	{
		// TODO: If we don't have this sleep here the device gets into a bad state. It seems either
		// resetting or closing the device while data is pending is a bad idea. We'll need to account
		// for this since it can happen in the wild if e.g. a user unplugs the device.
		WaitForResponse(ft232h);
		AssertEmptyReadBuffer(ft232h);
	}

	FT_STATUS status;

	// NOTE: Just following AN_135
	// Reset MPSSE controller
	UCHAR mask = 0;
	status = FT_SetBitMode(ft232h.device, mask, FT_BITMODE_RESET);
	LOG_IF(status != FT_OK, IGNORE,
		Severity::Error, "Failed to reset MPSSE: %", status);

	status = FT_Close(ft232h.device);
	LOG_IF(status != FT_OK, IGNORE,
		Severity::Error, "Failed to close device: %", status);

	List_Free(ft232h.pendingCommands);

	ft232h = {};
}
