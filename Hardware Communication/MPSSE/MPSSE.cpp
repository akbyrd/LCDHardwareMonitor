#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>

#include "ftd2xx.h"

#define Assert(condition) if (!(condition)) { *((int*) 0) = 0; }

using u8 = unsigned char;
using u16 = unsigned short;

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
	static const u8 SendBytesRisingMSBFirst     = '\x10';
	static const u8 SendRecvBytesRisingMSBFirst = '\x34';
	static const u8 SetDataBitsLowByte          = '\x80';
	static const u8 ReadDataBitsLowByte         = '\x81';
	static const u8 SetDataBitsHighByte         = '\x82';
	static const u8 EnableLoopback              = '\x84';
	static const u8 DisableLoopback             = '\x85';
	static const u8 SetClockDivisor             = '\x86';
	static const u8 DisableClockDivide          = '\x8A';
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

template <int N>
FT_STATUS Write(FT_HANDLE device, u8 (&data)[N])
{
	DWORD bytesWritten;
	FT_STATUS status = FT_Write(device, data, N, &bytesWritten);
	CheckStatus(status);
	Assert(bytesWritten == N);
	return status;
};

void AssertEmptyReadBuffer(FT_HANDLE device)
{
	DWORD bytesInReadBuffer;
	FT_STATUS status = FT_GetQueueStatus(device, &bytesInReadBuffer);
	CheckStatus(status);
	Assert(bytesInReadBuffer == 0);
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

	// HACK
	//Sleep(2 * 1);

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

	u8 buffer[2];
	DWORD numBytesRead = 0;
	status = FT_Read(device, buffer, ArrayLength(buffer), &numBytesRead);
	CheckStatus(status);
	Assert(bytesInReadBuffer == numBytesRead);

	Assert(buffer[0] == Response::BadCommand);
	Assert(buffer[1] == Command::BadCommand);

	Write(device, Command::DisableClockDivide);
	Write(device, Command::DisableAdaptiveClock);
	Write(device, Command::Disable3PhaseClock);

	#define BYTE(n, val) u8(((val) >> (n*8)) & 0xFF)

	// SCL Frequency = 60 Mhz / ((1 + 0x05DB) * 2) = 1 Mhz
	u16 clockDivisor = 0x05DB;
	u8 clockCmd[] = { Command::SetClockDivisor, BYTE(0, clockDivisor), BYTE(1, clockDivisor) };
	Write(device, clockCmd);

	// Pins 0-3 are SPI pins
	u8 pinValues = 0b1100'1001;
	u8 pinDirections = 0b1111'1011;
	u8 pinInitLCmd[] = { Command::SetDataBitsLowByte, pinValues, pinDirections };
	Write(device, pinInitLCmd);

	pinValues = 0b0000'0000;
	pinDirections = 0b0000'0000;
	u8 pinInitHCmd[] = { Command::SetDataBitsHighByte, pinValues, pinDirections };
	Write(device, pinInitHCmd);

	// Output on rising edge, other device will read on falling edge
	u8 dataOut[] = { 0xA5, 0x0F };
	u16 dataSize = u16(ArrayLength(dataOut));
	u8 dataCmd[] = { Command::SendBytesRisingMSBFirst, BYTE(0, dataSize - 1), BYTE(1, dataSize - 1) };
	Write(device, dataCmd);
	Write(device, dataOut);

	// Wait for the status to be returned
	Sleep(2 * latency);

	// We only clocked data out, not in
	status = FT_GetQueueStatus(device, &bytesInReadBuffer);
	CheckStatus(status);
	AssertEmptyReadBuffer(device);

	u8 dataCmd2[] = { Command::SendRecvBytesRisingMSBFirst, BYTE(0, dataSize - 1), BYTE(1, dataSize - 1) };
	Write(device, dataCmd2);
	Write(device, dataOut);

	// Wait for the status to be returned
	Sleep(2 * latency);

	// Now we should have actual data
	status = FT_GetQueueStatus(device, &bytesInReadBuffer);
	CheckStatus(status);

	u8 dataIn[2];
	status = FT_Read(device, dataIn, bytesInReadBuffer, &numBytesRead);
	CheckStatus(status);
	Assert(bytesInReadBuffer == numBytesRead);

	Assert(dataIn[0] == dataOut[0]);
	Assert(dataIn[1] == dataOut[1]);

	Write(device, Command::DisableLoopback);
	// NOTE: Just following AN_135
	AssertEmptyReadBuffer(device);

	Write(device, Command::ReadDataBitsLowByte);
	Sleep(2 * latency);
	status = FT_GetQueueStatus(device, &bytesInReadBuffer);
	CheckStatus(status);
	Assert(bytesInReadBuffer == 1);
	u8 gpio;
	status = FT_Read(device, &gpio, bytesInReadBuffer, &numBytesRead);
	Assert(bytesInReadBuffer == numBytesRead);

	// Looks like input pins are pulled up
	// I don't think setting the value of an input pin has any effect
	// I don't think setting the value of first 4 low pins has any effect because those are SPI lines

	// Low  pinValues     = 0b1100'1001;
	// Low  pinDirections = 0b1111'1011;
	// High pinValues     = 0b0000'0000;
	// High pinDirections = 0b0000'0000;
	//
	// Low  0b1100'1111
	// High 0b1111'1111

	// TODO: I guess you can't read the pin directions?
	pinValues = gpio & 0b1111'0111;
	pinDirections = 0b1111'1011; // Same as what we set above
	u8 pinSetLowCmd[] = { Command::SetDataBitsLowByte, pinValues, pinDirections };
	Write(device, pinSetLowCmd);
	Sleep(2 * latency);

	Write(device, Command::ReadDataBitsLowByte);
	Sleep(2 * latency);
	status = FT_GetQueueStatus(device, &bytesInReadBuffer);
	CheckStatus(status);
	Assert(bytesInReadBuffer == 1);
	status = FT_Read(device, &gpio, bytesInReadBuffer, &numBytesRead);
	Assert(bytesInReadBuffer == numBytesRead);

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

// Interfaces
// 	UART                     12 mbaud (default)
// 	FT245 Synchronous FIFO   40 mBps
// 	FT245 Asynchronous FIFO  8 mBps
// 	Async/Sync Bit-Bang
// 	MPSSE (JTAG SPI I2C)     30 mbps
// 	Fast Serial              12 mbps, opto-isolated
// 	CPU-Style FIFO
// 	FT1248

// Device
// 	I/O is 3.3V but 5V tolerant

// 60 FPS: 8.79 mBps (70 mbps)

// Chip Basics
// https://www.ftdichip.com/Support/Documents/DataSheets/ICs/DS_FT232H.pdf

// MPSSE Basics
// http://www.ftdichip.com/Support/Documents/AppNotes/AN_135_MPSSE_Basics.pdf

// MPSSE Commands
// https://www.ftdichip.com/Support/Documents/AppNotes/AN_108_Command_Processor_for_MPSSE_and_MCU_Host_Bus_Emulation_Modes.pdf

// Adaptive clocking: I don't think we need it (for ARM processors)
// Mode set: Read from (external) EEPROM on reset
// 	Is this different than regular EEPROM? There's a separate tool
// 	http://www.ftdichip.com/Resources/Utilities/FT_Prog_v1.4.zip
// 	FT_SetBitMode to change at runtime

// MPSSE
// 	CLKOUT=30MHz
// 	MPSSE mode is enabled using FT_SetBitMode with value 0x02
// 	Reset: FT_SetBitMode 0x00
// 	Master-only
// 	GPIOL1 is reserved/used in SPI
// 	MOSI/MISO have pull-ups, can be left unconnected
// 	Everything is sent over plain read/write calls to D2XX
// 	If the host and slave have different endianess might have to toggle MSB/LSB for read/write
// 	Both byte and bit based commands available
// 	Commands operate on bytes, MSB
// 	60 MHz clock. Speed = 60 / ((1 + Div) * 2) (probably)
// 	Clock div 5 enabled by default
// 	Can send on rising or falling edges. Need to check screen.
// 	Initial state of SK/TCK determines clock order
// 	Multiple commands can be sent in a single FT_Write? Not sure what the means exactly.
// 	Write buffer: 1 B command, 2 B data length, 1 - 65536 B payload
// 	Commands: op-code then 1+ parameters
// 	Bad command returns 0xFA
// 	Intentionally write bad command and check for 0xFA
// 	GPIO read: FT_Write command then FT_Read
// 	Docs looks like they reset MPSSE before closing
// 	If sent individually, as demonstrated below, the speed at which the GPIO signals can be
// 		changed is limited to the USB bus sending each FT_Write callin a separate transaction. This
// 		is one of the cases where it is desirable to chain multiple MPSSE commands into a single
// 		FT_Write call. The CS can be set, data transferred and CS cleared with one call.

// For example code below
// 	Set initial states of the MPSSE interface
// 		low byte, both pin directions and output values
// 		Pin name    Signal    Direction    Config    Initial State    Config
// 		ADBUS0      TCK/SK    output       1         high             1
// 		ADBUS1      TDI/DO    output       1         low              0
// 		ADBUS2      TDO/DI    input        0                          0
// 		ADBUS3      TMS/CS    output       1         high             1
// 		ADBUS4      GPIOL0    output       1         low              0
// 		ADBUS5      GPIOL1    output       1         low              0
// 		ADBUS6      GPIOL2    output       1         high             1
// 		ADBUS7      GPIOL3    output       1         high             1
// 	Note that since the data in subsequent sections will be clocked on the rising edge, the inital
// 		clock state of high is selected. Clocks will be generated as high-low-high. For example, in
// 		this case, data changes on the rising edge to give it enough time to have it available at
// 		the device, which will accept data *into* the target device on the falling edge.
// 		ACBUS0      GPIOH0    input        0                          0
// 		ACBUS1      GPIOH1    input        0                          0
// 		ACBUS2      GPIOH2    input        0                          0
// 		ACBUS3      GPIOH3    input        0                          0
// 		ACBUS4      GPIOH4    input        0                          0
// 		ACBUS5      GPIOH5    input        0                          0
// 		ACBUS6      GPIOH6    input        0                          0
// 		ACBUS7      GPIOH7    input        0                          0

// AN_135 code
// Open Device
// 	FT_CreateDeviceInfoList
// 	FT_Open
// MPSSE
// 	FT_ResetDevice      // No explanation
// 	FT_GetQueueStatus   // Flush via read
// 	FT_Read             // Flush via read
// 	FT_SetUSBParameters // (65536, 65535)
// 	FT_SetChars         // (false, 0, false, 0)
// 	FT_SetTimeouts      // (0, 5000)
// 	FT_SetLatencyTimer  // (1) (default: 16)
// 	FT_SetFlowControl   // (FT_FLOW_RTS_CTS, 0x00, 0x00) Turn on flow control to synchronize IN requests
// 	FT_SetBitMode       // (0x0, 0x00) Reset controller
// 	FT_SetBitMode       // (0x0, 0x02) Enable MPSSE mode
// 	Sleep(50)           // Wait for all the USB stuff to complete and work
// Loopback
// 	FT_Write            // (0x84) Enable loopback
// 	FT_GetQueueStatus   // Check the receive buffer - it should be empty
// 	FT_Write            // (0xAB) Sync via bogus command
// 	FT_Read             // Check for 0xFA 0xAB (weird loop)
// 	FT_Write            // (0x85) Disable loopback
// 	FT_GetQueueStatus   // Check the receive buffer - it should be empty
// JTAG (Hi-Speed)
// 	FT_Write            // (0x8A) Use 60MHz master clock (disable divide by 5)
// 	FT_Write            // (0x97) Turn off adaptive clocking (may be needed for ARM)
// 	FT_Write            // (0x8D) Disable three-phase clocking
// JTAG (TCK Frequency)
// TCK = 60MHz /((1 + [(1 +0xValueH*256) OR 0xValueL])*2)
// 	FT_Write            // (\x86) Set clock divisor command
// 	FT_Write            // (dwClockDivisor & 0xFF) Set 0xValueL of clock divisor
// 	FT_Write            // (dwClockDivisor >>8) & 0xFF Set 0xValueH of clock divisor
// MPSSE Initial States
// 	FT_Write            // (0x80) Configure data bits low-byte of MPSSE port
// 	FT_Write            // (0xC9) Initial state config above
// 	FT_Write            // (0xFB) Direction config above
// 	FT_Write            // (0x82) Configure data bits low-byte of MPSSE port
// 	FT_Write            // (0x00) Initial state config above
// 	FT_Write            // (0x00) Direction config above
// Serial Transmission 1
// 	FT_Write            // (0x10) Output on rising clock, no input, MSB first, clock a number of bytes out
// 	FT_Write            // (0x01) Length L
// 	FT_Write            // (0x00) Length H, Length = 0x0001 + 1
// 	FT_Write            // (0xA5) Data = 0xA50F
// 	FT_Write            // (0x0F) Data = 0xA50F
// 	Sleep(2)            // Wait for data to be transmitted and status to be returnedby the device driver. see latency timer above
// 	FT_GetQueueStatus   // Check the receive buffer - it should be empty
// Serial Transmission 2
// 	FT_Write            // (0x34) Output on rising clock, input on falling clock, MSB first, clock a number of bytes out
// 	FT_Write            // (0x01) Length L
// 	FT_Write            // (0x00) Length H, Length = 0x0001 + 1
// 	FT_Write            // (0xA5) Data = 0xA50F
// 	FT_Write            // (0x0F) Data = 0xA50F
// 	Sleep(2)
// 	FT_GetQueueStatus   // The input buffer should contain the same number of bytes as those output (loopback enabled!)
// 	FT_Read
// GPIO
// 	FT_Write            // (0x81) Get data bits, returns state of pins, either input or output on low byte of MPSSE
// 	Sleep(2)
// 	FT_GetQueueStatus
// 	FT_Read
// 	FT_Write            // (0x80) Set data bits low-byte of MPSSE port
// 	FT_Write            // (byInputBuffer[0] & 0xF7) Only change TMS/CS bit to zero
// 	FT_Write            // (0xFB) Direction config is still needed for each GPIO write
// 	Sleep(2)
// Close Device
// 	FT_SetBitMode       // (0x0, 0x00) Reset controller
// 	FT_Close
