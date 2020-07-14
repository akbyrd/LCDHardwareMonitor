#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdint>

#include "ftd2xx.h"

#define Assert(condition) if (!(condition)) { *((int*) 0) = 0; }

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

int main()
{
	// SetBaudRate
	// SetDataCharacteristics
	// SetTimeouts
	// SetFlowControl
	// SetDtr
	// SetRts
	// SetBreakOn/Off

	// Read
	// Write

#define CheckStatus(status, errorCode) \
		if (status != FT_OK) \
		{ \
			HandleError(status); \
			return errorCode; \
		}

#define Trace(msg) \
		OutputDebugStringA(msg);

	FT_STATUS status = -1;
	int errorCode = -1;

	// NOTE: Device list only updates when calling this function
	DWORD deviceCount;
	status = FT_CreateDeviceInfoList(&deviceCount);
	CheckStatus(status, errorCode--);
	Trace("Obtained device count\n");

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
	CheckStatus(status, errorCode--);
	Trace("Obtained device list\n");

	if (deviceInfoCount > 0)
	{
		DeviceFlags deviceFlags;
		deviceFlags.raw = deviceInfos[0].Flags;
		int x = 0;
	}

	FT_HANDLE device = nullptr;
	status = FT_Open(0, &device);
	CheckStatus(status, errorCode--);
	Trace("Opened device\n");

	for (int i = 0; i < 100; i++)
	{
		int writeBytes = i;
		DWORD desiredWriteCount = sizeof(writeBytes);
		DWORD actualWriteCount;
		status = FT_Write(device, &writeBytes, desiredWriteCount, &actualWriteCount);
		CheckStatus(status, errorCode--);

		int readBytes;
		DWORD desiredReadCount = sizeof(readBytes);
		DWORD actualReadCount;
		status = FT_Read(device, &readBytes, desiredReadCount, &actualReadCount);
		CheckStatus(status, errorCode--);

		// Weirdo non-standard error condition
		if (actualReadCount == 0 && desiredReadCount != 0)
		{
			Trace("Read timed out\n");
			return errorCode;
		}
		errorCode--;

		Assert(readBytes == writeBytes);
	}

	status = FT_Close(device);
	CheckStatus(status, errorCode--);
	Trace("Closed device\n");

	return 0;
}

// NOTE: VS is automatically linking the lib because it's included in the project

// TODO: How do we do GPIO?
// level_low  = (self._level & 0xFF)
// level_high = ((self._level >> 8) & 0xFF)
// dir_low  = (self._direction & 0xFF)
// dir_high = ((self._direction >> 8) & 0xFF)
// return bytes(bytearray((0x80, level_low, dir_low, 0x82, level_high, dir_high)))