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
	// Hello
	//Close
	//OpenEx or Open
	//ResetDevice
	//Purge(FT_PURGE_RX || FT_PURGE_TX)
	//ResetDevice
	//SetTimeouts
	//Sleep(150);
	//Write
	//Read
	//SetTimeouts

	// Loadem
	//OpenEx or Open
	//ResetDevice
	//Purge(FT_PURGE_RX || FT_PURGE_TX)
	//Write
	//Read
	//SetTimeouts

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

	FT_DEVICE_LIST_INFO_NODE deviceInfo = {};
	status = FT_GetDeviceInfo(device, &deviceInfo.Type, &deviceInfo.ID, deviceInfo.SerialNumber, deviceInfo.Description, nullptr);
	// TODO: Check device type
	CheckStatus(status, errorCode--);
	Trace("Obtained device info\n");
	WORD _vid = (deviceInfo.ID >> 16) & WORD(-1);
	WORD _pid = (deviceInfo.ID >> 00) & WORD(-1);

	DWORD receiveQueueLen;
	DWORD transmitQueueLen;
	DWORD eventStatus;
	status = FT_GetStatus(device, &receiveQueueLen, &transmitQueueLen, &eventStatus);
	CheckStatus(status, errorCode--);
	Trace("Obtained device status\n");

	union VersionNumber
	{
		struct
		{
			uint8_t build : 8;
			uint8_t minor : 8;
			uint8_t major : 8;
		};
		uint32_t raw;
	};
	static_assert(sizeof(VersionNumber) == sizeof(DWORD), "");

	VersionNumber driverVersion;
	status = FT_GetDriverVersion(device, (DWORD*) &driverVersion);
	CheckStatus(status, errorCode--);
	Trace("Obtained driver version\n");

	VersionNumber dllVersion;
	status = FT_GetLibraryVersion((DWORD*) &dllVersion);
	CheckStatus(status, errorCode--);
	Trace("Obtained dll version\n");

	DWORD receiveQueueLen2;
	status = FT_GetQueueStatus(device, &receiveQueueLen2);
	CheckStatus(status, errorCode--);
	Trace("Obtained queue status\n");

	enum struct ModemStatus : uint8_t
	{
		CTS = 0x10, // Clear To Send
		DSR = 0x20, // Data Set Ready
		RI  = 0x40, // Ring Indicator
		DCD = 0x80, // Data Carrier Detect
	};

	enum struct LineStatus : uint8_t
	{
		OE = 0x02, // Overrun Error
		PE = 0x04, // Parity Error
		FE = 0x08, // Framing Error
		BI = 0x10, // BreakInterrupt
	};

	union ModemLineStatus
	{
		struct
		{
			ModemStatus modem : 8;
			LineStatus  line  : 8;
		};
		uint32_t raw;
	};
	static_assert(sizeof(ModemLineStatus) == sizeof(ULONG), "");

	// NOTE: Line starts with non-zero errors. Is a reset required?
	ModemLineStatus modemLineStatus;
	status = FT_GetModemStatus(device, (ULONG*) &modemLineStatus);
	CheckStatus(status, errorCode--);
	Trace("Obtained modem status\n");

	LONG comPort;
	status = FT_GetComPortNumber(device, &comPort);
	CheckStatus(status, errorCode--);
	Trace("Obtained COM port\n");

	char ManufacturerBuffer[32];
	char ManufacturerIdBuffer[16];
	char DescriptionBuffer[64];
	char SerialNumberBuffer[16];

	FT_PROGRAM_DATA eeprom;
	eeprom.Signature1     = 0x00000000;
	eeprom.Signature2     = 0xffffffff;
	eeprom.Version        = 9;
	eeprom.Manufacturer   = ManufacturerBuffer;
	eeprom.ManufacturerId = ManufacturerIdBuffer;
	eeprom.Description    = DescriptionBuffer;
	eeprom.SerialNumber   = SerialNumberBuffer;
	status = FT_EE_Read(device, &eeprom);
	CheckStatus(status, errorCode--);
	Trace("Read EEPROM\n");

	status = FT_Close(device);
	CheckStatus(status, errorCode--);
	Trace("Closed device\n");

	return 0;
}

// TODO: How is this using the lib? I didn't specify it...
