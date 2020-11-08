struct ILI9341State
{
	// TODO: Consider removing this
	FT232HState* ft232h;

	b8    enableSPIStrict;
	b8    inTransaction;

	b8    sleep;
	i64   sleepTime;
	i64   wakeTime;
	b8    rowColSwap;
	v2u16 size;
	b8    drawingFrames;
};

namespace ILI9341
{
	struct Command
	{
		static const u8 Nop                          = 0x00;
		static const u8 SoftwareReset                = 0x01;
		static const u8 ReadIdentificationInfo       = 0x04;
		static const u8 ReadStatus                   = 0x09;
		static const u8 ReadPowerMode                = 0x0A;
		static const u8 ReadMemoryAccessControl      = 0x0B;
		static const u8 ReadPixelFormat              = 0x0C;
		static const u8 ReadImageFormat              = 0x0D;
		static const u8 ReadSignalMode               = 0x0E;
		static const u8 ReadSelfDiagnostic           = 0x0F;
		static const u8 SleepIn                      = 0x10;
		static const u8 SleepOut                     = 0x11;
		static const u8 GammaSet                     = 0x26;
		static const u8 DisplayOff                   = 0x28;
		static const u8 DisplayOn                    = 0x29;
		static const u8 ColumnAddressSet             = 0x2A;
		static const u8 PageAddressSet               = 0x2B;
		static const u8 MemoryWrite                  = 0x2C;
		static const u8 MemoryAccessControl          = 0x36;
		static const u8 PixelFormatSet               = 0x3A;
		static const u8 FrameRateControlNormal       = 0xB1;
		static const u8 DisplayFunctionControl       = 0xB6;
		static const u8 PowerControl1                = 0xC0;
		static const u8 PowerControl2                = 0xC1;
		static const u8 VCOMControl1                 = 0xC5;
		static const u8 VCOMControl2                 = 0xC7;
		static const u8 PowerControlA                = 0xCB;
		static const u8 PowerControlB                = 0xCF;
		static const u8 PositiveGammaCorrection      = 0xE0;
		static const u8 NegativeGammaCorrection      = 0xE1;
		static const u8 DriverTimingControlAInternal = 0xE8;
		static const u8 DriverTimingControlAExternal = 0xE9;
		static const u8 DriverTimingControlB         = 0xEA;
		static const u8 PowerOnSequenceControl       = 0xED;
		static const u8 Enable3Gamma                 = 0xF2;
		static const u8 PumpRatioControl             = 0xF7;
	};
}

// -------------------------------------------------------------------------------------------------
// Public API - Fundamental Functions

void
ILI9341_BeginWriteTransaction(ILI9341State& ili9341)
{
	Assert(!ili9341.inTransaction)
	ili9341.inTransaction = true;

	if (ili9341.enableSPIStrict)
		FT232H_BeginSPITransaction(*ili9341.ft232h);
}

void
ILI9341_EndWriteTransaction(ILI9341State& ili9341)
{
	Assert(ili9341.inTransaction);
	ili9341.inTransaction = false;

	if (ili9341.enableSPIStrict)
		FT232H_EndSPITransaction(*ili9341.ft232h);
}

void
ILI9341_WriteCmdRaw(ILI9341State& ili9341, u8 cmd)
{
	Assert(ili9341.inTransaction);
	FT232H_SetDC(*ili9341.ft232h, Signal::Low);
	FT232H_SendBytes(*ili9341.ft232h, cmd);
}

void
ILI9341_WriteDataRaw(ILI9341State& ili9341, ByteSlice bytes)
{
	Assert(ili9341.inTransaction);
	FT232H_SetDC(*ili9341.ft232h, Signal::High);
	FT232H_SendBytes(*ili9341.ft232h, bytes);
}

void
ILI9341_WriteCmd(ILI9341State& ili9341, u8 cmd)
{
	Assert(!ili9341.inTransaction);

	if (ili9341.enableSPIStrict)
	{
		FT232H_BeginSPITransaction(*ili9341.ft232h);
		FT232H_SetDC(*ili9341.ft232h, Signal::Low);
		FT232H_SendBytes(*ili9341.ft232h, cmd);
		FT232H_EndSPITransaction(*ili9341.ft232h);
		FT232H_SetDO(*ili9341.ft232h, Signal::Low);
		FT232H_SetDC(*ili9341.ft232h, Signal::High);
	}
	else
	{
		FT232H_SetDC(*ili9341.ft232h, Signal::Low);
		FT232H_SendBytes(*ili9341.ft232h, cmd);
	}
}

void
ILI9341_WriteData(ILI9341State& ili9341, ByteSlice bytes)
{
	Assert(!ili9341.inTransaction);

	if (ili9341.enableSPIStrict)
	{
		FT232H_BeginSPITransaction(*ili9341.ft232h);
		FT232H_SetDC(*ili9341.ft232h, Signal::High);
		FT232H_SendBytes(*ili9341.ft232h, bytes);
		FT232H_EndSPITransaction(*ili9341.ft232h);
		FT232H_SetDO(*ili9341.ft232h, Signal::Low);
	}
	else
	{
		FT232H_SetDC(*ili9341.ft232h, Signal::High);
		FT232H_SendBytes(*ili9341.ft232h, bytes);
	}
}

void
ILI9341_Write(ILI9341State& ili9341, u8 cmd, ByteSlice bytes)
{
	Assert(!ili9341.inTransaction);

	if (ili9341.enableSPIStrict)
	{
		FT232H_BeginSPITransaction(*ili9341.ft232h);
		FT232H_SetDC(*ili9341.ft232h, Signal::Low);
		FT232H_SendBytes(*ili9341.ft232h, cmd);
		FT232H_SetDC(*ili9341.ft232h, Signal::High);
		FT232H_SendBytes(*ili9341.ft232h, bytes);
		FT232H_EndSPITransaction(*ili9341.ft232h);
		FT232H_SetDO(*ili9341.ft232h, Signal::Low);
	}
	else
	{
		FT232H_SetDC(*ili9341.ft232h, Signal::Low);
		FT232H_SendBytes(*ili9341.ft232h, cmd);
		FT232H_SetDC(*ili9341.ft232h, Signal::High);
		FT232H_SendBytes(*ili9341.ft232h, bytes);
	}
}

// TODO: Don't want to change the read speed for individual reads.
void
ILI9341_Read(ILI9341State& ili9341, u8 cmd, Bytes& bytes, u16 numBytesToRead)
{
	Assert(!ili9341.inTransaction);

	FT232H_SetClockSpeed(*ili9341.ft232h, 15'000'000);
	FT232H_BeginSPITransaction(*ili9341.ft232h);
	FT232H_SetDC(*ili9341.ft232h, Signal::Low);
	FT232H_SendBytes(*ili9341.ft232h, cmd);
	FT232H_RecvBytes(*ili9341.ft232h, bytes, numBytesToRead);
	FT232H_EndSPITransaction(*ili9341.ft232h);
	FT232H_SetClockSpeed(*ili9341.ft232h, 30'000'000);

	if (ili9341.enableSPIStrict)
	{
		FT232H_SetDO(*ili9341.ft232h, Signal::Low);
		FT232H_SetDC(*ili9341.ft232h, Signal::High);
	}
}

// -------------------------------------------------------------------------------------------------
// Internal Configuration Functions

void
ILI9341_SetSPIStrict(ILI9341State& ili9341, b8 enable)
{
	ili9341.enableSPIStrict = enable;
}

void
ILI9341_MemoryAccessControl(ILI9341State& ili9341)
{
	// NOTE: I have no idea why bits 3 (rgb) and 6 (column order) have to be flipped, but every other
	// piece of code I can find does the same, including an example program from the manufacturer of
	// one of these screens. Despite the documentation, I guess the hardware defaults to top right
	// and BGR? Also, note that landscape layout requires swapping rows and columns and flipping the
	// column order, so the two column order swaps cancel out.

	b8 portrait = false;

	// Landscape layout
	u8 rowAddressOrder_TopToBottom        = 0 << 7;
	u8 colAddressOrder_LeftToRight        = 1 << 6;
	u8 colAddressOrder_RightToLeft        = 0 << 6;
	u8 rowColExchange_No                  = 0 << 5;
	u8 rowColExchange_Yes                 = 1 << 5;
	u8 verticalRefreshOrder_TopToBottom   = 0 << 4;
	u8 rgbOrder_RGB                       = 1 << 3;
	u8 horizontalRefershOrder_LeftToRight = 0 << 2;

	u8 mac = 0;
	mac |= rowAddressOrder_TopToBottom;
	mac |= portrait ? colAddressOrder_LeftToRight : colAddressOrder_RightToLeft;
	mac |= portrait ? rowColExchange_No : rowColExchange_Yes;
	mac |= verticalRefreshOrder_TopToBottom;
	mac |= rgbOrder_RGB;
	mac |= horizontalRefershOrder_LeftToRight;
	ILI9341_Write(ili9341, ILI9341::Command::MemoryAccessControl, mac);

	if (portrait)
	{
		ili9341.rowColSwap = false;
		ili9341.size = { 240, 320 };
	}
	else
	{
		ili9341.rowColSwap = true;
		ili9341.size = { 320, 240 };
	}
}

void
ILI9341_PixelFormatSet(ILI9341State& ili9341)
{
	u8 bpp16 = 0b101;
	//u8 bpp18 = 0b110;

	u8 mcuShift = 0;
	u8 rgbShift = 4;

	u8 pf = 0;
	pf |= bpp16 << mcuShift;
	pf |= bpp16 << rgbShift;

	ILI9341_Write(ili9341, ILI9341::Command::PixelFormatSet, pf);
}

void
ILI9341_FrameRateControl_Normal(ILI9341State& ili9341)
{
	u8 divisionRatio_1 = 0b00;
	//u8 divisionRatio_2 = 0b01;
	//u8 divisionRatio_4 = 0b10;
	//u8 divisionRatio_8 = 0b11;

	u8 frameRate_119 = 0b10000;
	//u8 frameRate_112 = 0b10001;
	//u8 frameRate_106 = 0b10010;
	//u8 frameRate_100 = 0b10011;
	//u8 frameRate_95  = 0b10100;
	//u8 frameRate_90  = 0b10101;
	//u8 frameRate_86  = 0b10110;
	//u8 frameRate_83  = 0b10111;
	//u8 frameRate_79  = 0b11000;
	//u8 frameRate_76  = 0b11001;
	//u8 frameRate_73  = 0b11010;
	//u8 frameRate_70  = 0b11011;
	//u8 frameRate_68  = 0b11100;
	//u8 frameRate_65  = 0b11101;
	//u8 frameRate_63  = 0b11110;
	//u8 frameRate_61  = 0b11111;

	u8 frc_divisionRatio = divisionRatio_1;
	u8 frc_clocksPerLine = frameRate_119;
	u8 params[] = { frc_divisionRatio, frc_clocksPerLine };
	ILI9341_Write(ili9341, ILI9341::Command::FrameRateControlNormal, params);
}

void
ILI9341_DisplayFunctionControl(ILI9341State& ili9341)
{
	// NOTE: Display Function Control defaults are fine.
	Unused(ili9341);
}

void
ILI9341_PowerControl(ILI9341State& ili9341)
{
	// NOTE: Power Control 1 defaults are fine.
	// NOTE: Power Control 2 defaults are fine.
	// NOTE: Power Control A defaults are fine.

	u8 pcb_p1_default = 0x00;
	u8 pcb_p2_default = 0x81;
	u8 pcb_p2_pceq_enable = 1 << 6;

	u8 pcb_p1 = pcb_p1_default;
	u8 pcb_p2 = (u8) (pcb_p2_default | pcb_p2_pceq_enable);

	u8 params[] = { pcb_p1, pcb_p2 };
	ILI9341_Write(ili9341, ILI9341::Command::PowerControlB, params);
	// NOTE: Leave parameter 3 at default
	// TODO: Is this necessary if we use SPI transactions? I think raising CS will do the same thing
	ILI9341_WriteCmd(ili9341, ILI9341::Command::Nop);
}

void
ILI9341_VCOMControl(ILI9341State& ili9341)
{
	// NOTE: VCOM Control 1 defaults are fine.
	// NOTE: VCOM Control 2 defaults are fine.
	Unused(ili9341);
}

void
ILI9341_DriverTimingControl(ILI9341State& ili9341)
{
	// NOTE: Driver Timing Control A (1) defaults are fine
	// NOTE: Driver Timing Control A (2) isn't used by any example code

	// NOTE: Just doing what everyone else does...
	u8 params[] = { 0x00, 0x00 };
	ILI9341_Write(ili9341, ILI9341::Command::DriverTimingControlB, params);
}

void
ILI9341_PowerOnSequenceControl(ILI9341State& ili9341)
{
	// NOTE: Just doing what everyone else does...
	u8 params[] = { 0x64, 0x03, 0x12, 0x81 };
	ILI9341_Write(ili9341, ILI9341::Command::PowerOnSequenceControl, params);
}

void
ILI9341_PumpRatioControl(ILI9341State& ili9341)
{
	u8 ratioControl_2x = 0b10 << 4;

	u8 prc_p1 = 0;
	prc_p1 |= ratioControl_2x;
	ILI9341_Write(ili9341, ILI9341::Command::PumpRatioControl, prc_p1);
}

void
ILI9341_SetGamma(ILI9341State& ili9341)
{
	// NOTE: Enable 3 Gamma default is fine
	// NOTE: Gamma Set default is fine

	// NOTE: Just doing what everyone else does...
	u8 params1[] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
	u8 params2[] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
	ILI9341_Write(ili9341, ILI9341::Command::PositiveGammaCorrection, params1);
	ILI9341_Write(ili9341, ILI9341::Command::NegativeGammaCorrection, params2);
}

void
ILI9341_SetColumnAddress(ILI9341State& ili9341, v2u16 xRange)
{
	u8 params[] = { UnpackMSB2(xRange.pos), UnpackMSB2((u32) (xRange.pos + xRange.size - 1)) };
	ILI9341_Write(ili9341, ILI9341::Command::ColumnAddressSet, params);
}

void
ILI9341_SetRowAddress(ILI9341State& ili9341, v2u16 yRange)
{
	u8 params[] = { UnpackMSB2(yRange.pos), UnpackMSB2((u32) (yRange.pos + yRange.size - 1)) };
	ILI9341_Write(ili9341, ILI9341::Command::PageAddressSet, params);
}

void
ILI9341_SetWriteAddress(ILI9341State& ili9341, v4u16 rect)
{
	u8 params1[] = { UnpackMSB2(rect.pos.x), UnpackMSB2((u32) (rect.pos.x + rect.size.x - 1)) };
	u8 params2[] = { UnpackMSB2(rect.pos.y), UnpackMSB2((u32) (rect.pos.y + rect.size.y - 1)) };
	ILI9341_Write(ili9341, ILI9341::Command::ColumnAddressSet, params1);
	ILI9341_Write(ili9341, ILI9341::Command::PageAddressSet, params2);
}

void
ILI9341_ReadIdentificationInfo(ILI9341State& ili9341, Bytes& bytes)
{
	ILI9341_Read(ili9341, ILI9341::Command::ReadIdentificationInfo, bytes, 3);
}

void
ILI9341_ReadStatus(ILI9341State& ili9341, Bytes& bytes)
{
	ILI9341_Read(ili9341, ILI9341::Command::ReadStatus, bytes, 4);
}

void
ILI9341_ReadPowerMode(ILI9341State& ili9341, Bytes& bytes)
{
	ILI9341_Read(ili9341, ILI9341::Command::ReadPowerMode, bytes, 1);
}

void
ILI9341_ReadMemoryAccessControl(ILI9341State& ili9341, Bytes& bytes)
{
	ILI9341_Read(ili9341, ILI9341::Command::ReadMemoryAccessControl, bytes, 1);
}

void
ILI9341_ReadPixelFormat(ILI9341State& ili9341, Bytes& bytes)
{
	ILI9341_Read(ili9341, ILI9341::Command::ReadPixelFormat, bytes, 1);
}

void
ILI9341_ReadImageFormat(ILI9341State& ili9341, Bytes& bytes)
{
	ILI9341_Read(ili9341, ILI9341::Command::ReadImageFormat, bytes, 1);
}

void
ILI9341_ReadSignalMode(ILI9341State& ili9341, Bytes& bytes)
{
	ILI9341_Read(ili9341, ILI9341::Command::ReadSignalMode, bytes, 1);
}

void
ILI9341_ReadSelfDiagnostic(ILI9341State& ili9341, Bytes& bytes)
{
	ILI9341_Read(ili9341, ILI9341::Command::ReadSelfDiagnostic, bytes, 1);
}

// -------------------------------------------------------------------------------------------------
// Public API - Logic Functions

void
ILI9341_Reset(ILI9341State& ili9341)
{
	//NOTE: Resets device state to defaults
	ILI9341_WriteCmd(ili9341, ILI9341::Command::SoftwareReset);
	ili9341.sleep = true;
	ili9341.sleepTime = Platform_GetTicks();
	Platform_Sleep(5);
	//ili9341.nextCommandTime = ili9341.sleepTime + 5ms
}

void
ILI9341_Sleep(ILI9341State& ili9341)
{
	// NOTE: MCU and memory are still working. Memory maintains contents.
	Assert(!ili9341.sleep);

	u32 sinceWake = (u32) Platform_GetElapsedMilliseconds(ili9341.wakeTime);
	if (sinceWake < 120)
			Platform_Sleep(120 - sinceWake);

	ILI9341_WriteCmd(ili9341, ILI9341::Command::SleepIn);
	ili9341.sleep = true;
	ili9341.sleepTime = Platform_GetTicks();
	Platform_Sleep(5);
	//ili9341.nextCommandTime = ili9341.sleepTime + 5ms;
}

void
ILI9341_Wake(ILI9341State& ili9341)
{
	Assert(ili9341.sleep);

	u32 sinceSleep = (u32) Platform_GetElapsedMilliseconds(ili9341.sleepTime);
	if (sinceSleep < 120)
		Platform_Sleep(120 - sinceSleep);

	ILI9341_WriteCmd(ili9341, ILI9341::Command::SleepOut);
	ili9341.sleep = false;
	ili9341.wakeTime = Platform_GetTicks();
	// NOTE: Docs contradict themselves. Is it 5 ms or 120 ms?
	Platform_Sleep(5);
	//ili9341.nextCommandTime = ili9341.wakeTime + 5ms
}

void
ILI9341_SetRect(ILI9341State& ili9341, v4u16 rect, u16 color)
{
	// NOTE: Each MemoryWrite command resets the page and column register to the previous set value
	// TODO: Maybe we should take advantage of that?

	Assert(rect.size.x != 0);
	Assert(rect.pos.x + rect.size.x <= ili9341.size.x);
	Assert(rect.size.y != 0);
	Assert(rect.pos.y + rect.size.y <= ili9341.size.y);
	ILI9341_SetWriteAddress(ili9341, rect);

	ILI9341_BeginWriteTransaction(ili9341);
	ILI9341_WriteCmdRaw(ili9341, ILI9341::Command::MemoryWrite);

	u32 totalDataLen = (u32) (2 * rect.size.x * rect.size.y);

	// NOTE: The FT232H has a maximum number of bytes it can send in a single command, so we only
	// need to make a buffer that big and can send it multiple times.
	u8 colorData[FT232H::MaxSendBytes];
	u32 colorDataLen = Min(totalDataLen, FT232H::MaxSendBytes);
	// TODO: Try using wmemset and profiling the difference
	for (u32 i = 0; i < colorDataLen; i += 2)
	{
		// TODO: Remove endian swap when using parallel interface
		colorData[i + 0] = GetByte(1, color);
		colorData[i + 1] = GetByte(0, color);
	}

	u32 remainingLen = totalDataLen;
	while (remainingLen > 0)
	{
		ByteSlice bytes = {};
		bytes.data   = colorData;
		bytes.length = Min(remainingLen, colorDataLen);
		bytes.stride = 1;

		ILI9341_WriteDataRaw(ili9341, bytes);
		remainingLen -= bytes.length;
	}
	ILI9341_EndWriteTransaction(ili9341);
}

void
ILI9341_SetPixel(ILI9341State& ili9341, v2u16 pos, u16 color)
{
	v4u16 rect = {};
	rect.pos  = pos;
	rect.size = { 1, 1 };
	ILI9341_SetRect(ili9341, rect, color);
}

void
ILI9341_DisplayTest(ILI9341State& ili9341)
{
	for (u16 i = 0; i < 120; i++)
	{
		u16 l = (u16) (0 + i);
		u16 r = (u16) (ili9341.size.x - 1 - i);
		u16 t = (u16) (0 + i);
		u16 b = (u16) (ili9341.size.y - 1 - i);
		ILI9341_SetPixel(ili9341, { l, t }, Colors16::Red);
		ILI9341_SetPixel(ili9341, { r, t }, Colors16::Green);
		ILI9341_SetPixel(ili9341, { l, b }, Colors16::Blue);
		ILI9341_SetPixel(ili9341, { r, b }, Colors16::Gray);
	}

	v2u16 quarter = ili9341.size / (u16) 4;

	if (ili9341.rowColSwap)
	{
		for (u16 i = 0; i < 80; i++)
		{
			u16 m = (u16) (120 + i);
			u16 t = (u16) (2 * quarter.y - 1);
			u16 b = (u16) (2 * quarter.y - 0);
			ILI9341_SetPixel(ili9341, { m, t }, Colors16::Black);
			ILI9341_SetPixel(ili9341, { m, b }, Colors16::Black);
		}
	}
	else
	{
		for (u16 i = 0; i < 80; i++)
		{
			u16 l = (u16) (2 * quarter.x - 1);
			u16 r = (u16) (2 * quarter.x - 0);
			u16 m = (u16) (120 + i);
			ILI9341_SetPixel(ili9341, { l, m }, Colors16::Black);
			ILI9341_SetPixel(ili9341, { r, m }, Colors16::Black);
		}
	}

	v4u16 lRect = MakeRect(v2u16{ 1, 2 } * quarter, (u16) 10);
	v4u16 rRect = MakeRect(v2u16{ 3, 2 } * quarter, (u16) 10);
	v4u16 tRect = MakeRect(v2u16{ 2, 1 } * quarter, (u16) 10);
	v4u16 bRect = MakeRect(v2u16{ 2, 3 } * quarter, (u16) 10);
	ILI9341_SetRect(ili9341, lRect, Colors16::Magenta);
	ILI9341_SetRect(ili9341, rRect, Colors16::Gray);
	ILI9341_SetRect(ili9341, tRect, Colors16::Yellow);
	ILI9341_SetRect(ili9341, bRect, Colors16::Cyan);
}

void
ILI9341_Clear(ILI9341State& ili9341, u16 color)
{
	v4u16 rect = {};
	rect.pos  = { 0, 0 };
	rect.size = ili9341.size;
	ILI9341_SetRect(ili9341, rect, color);
}

void
ILI9341_BeginDrawFrames(ILI9341State& ili9341)
{
	Assert(!ili9341.drawingFrames);
	ili9341.drawingFrames = true;

	u16 xMax = (u16) (ili9341.size.x - 1);
	u16 yMax = (u16) (ili9341.size.y - 1);
	u8 params1[] = { UnpackMSB2(0), UnpackMSB2(xMax) };
	u8 params2[] = { UnpackMSB2(0), UnpackMSB2(yMax) };
	ILI9341_Write(ili9341, ILI9341::Command::ColumnAddressSet, params1);
	ILI9341_Write(ili9341, ILI9341::Command::PageAddressSet, params2);
	ILI9341_WriteCmd(ili9341, ILI9341::Command::MemoryWrite);
	ILI9341_BeginWriteTransaction(ili9341);
}

void
ILI9341_EndDrawFrames(ILI9341State& ili9341)
{
	Assert(ili9341.drawingFrames);
	ili9341.drawingFrames = false;
	ILI9341_EndWriteTransaction(ili9341);
}

void
ILI9341_DrawFrame(ILI9341State& ili9341, ByteSlice bytes)
{
	Assert(ili9341.drawingFrames);
	Assert(!Slice_IsSparse(bytes));

	// TODO: Remove endian swap when using parallel interface
	Assert(bytes.length % 2 == 0);
	for (u32 i = 0; i < bytes.length; i += 2)
	{
		u8 temp = bytes[i];
		bytes[i + 0] = bytes[i + 1];
		bytes[i + 1] = temp;
	}

	ILI9341_WriteDataRaw(ili9341, bytes);
}

void
ILI9341_DrawSingleFrame(ILI9341State& ili9341, ByteSlice bytes)
{
	ILI9341_BeginDrawFrames(ili9341);
	ILI9341_DrawFrame(ili9341, bytes);
	ILI9341_EndDrawFrames(ili9341);
}

// -------------------------------------------------------------------------------------------------
// Public API - Core Functions

void
ILI9341_Initialize(ILI9341State& ili9341, FT232HState& ft232h)
{
	ili9341.ft232h = &ft232h;

	FT232H_SetCS(*ili9341.ft232h, Signal::Low);

	// TODO: Want to do a software reset to set all config to default, but it causes a flicker.
	ILI9341_Reset(ili9341);

	ILI9341_MemoryAccessControl(ili9341);
	ILI9341_PixelFormatSet(ili9341);
	ILI9341_FrameRateControl_Normal(ili9341);
	ILI9341_DisplayFunctionControl(ili9341);
	ILI9341_PowerControl(ili9341);
	ILI9341_VCOMControl(ili9341);
	ILI9341_DriverTimingControl(ili9341);
	ILI9341_PowerOnSequenceControl(ili9341);
	ILI9341_PumpRatioControl(ili9341);
	ILI9341_SetGamma(ili9341);
	ILI9341_WriteCmd(ili9341, ILI9341::Command::SleepOut);
	ILI9341_WriteCmd(ili9341, ILI9341::Command::DisplayOn);

	// TODO: The screen can do an endian swap, but only in the parallel interface. Once we're using
	// the parallel interface remove the byte-swaps in SetRect and DrawFrame and set the endianness
	// mode with Interface Control 0xF6.
}

void
ILI9341_Teardown(ILI9341State& ili9341)
{
	ili9341 = {};
}
