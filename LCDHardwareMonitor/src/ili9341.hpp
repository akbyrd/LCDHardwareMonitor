struct ILI9341State
{
	FT232HState* ft232h;

	b8  sleep;
	i64 sleepTime;
	i64 wakeTime;
	b8  rowColSwap;
	u16 xMax;
	u16 yMax;
	b8  drawingFrames;
};

namespace ILI9341
{
	struct Command
	{
		static const u8 Nop                          = 0x00;
		static const u8 SoftwareReset                = 0x01;
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
ILI9341_WriteCmd(ILI9341State& ili9341, u8 cmd)
{
	FT232H_SetDC(*ili9341.ft232h, Signal::Low);
	FT232H_SendBytes(*ili9341.ft232h, cmd);
}

void
ILI9341_WriteData(ILI9341State& ili9341, ByteSlice bytes)
{
	FT232H_SetDC(*ili9341.ft232h, Signal::High);
	FT232H_SendBytes(*ili9341.ft232h, bytes);
}

void
ILI9341_Write(ILI9341State& ili9341, u8 cmd, ByteSlice bytes)
{
	FT232H_SetDC(*ili9341.ft232h, Signal::Low);
	FT232H_SendBytes(*ili9341.ft232h, cmd);
	FT232H_SetDC(*ili9341.ft232h, Signal::High);
	FT232H_SendBytes(*ili9341.ft232h, bytes);
}

// TODO: Can this be constrained to u8 only?
template <typename ...Args>
void
ILI9341_Write(ILI9341State& ili9341, u8 cmd, Args... bytes)
{
	u8 data[] = { u8(bytes)... };
	ILI9341_WriteCmd(ili9341, cmd);
	ILI9341_WriteData(ili9341, data);
}

// -------------------------------------------------------------------------------------------------
// Internal Configuration Functions

void
ILI9341_MemoryAccessControl(ILI9341State& ili9341)
{
	// NOTE: I have no idea why bits 3 and 6 have to be flipped, but every other piece of code I can
	// find does the same, including an example program from the manufacturer of one of these
	// screens. Despite the documentation, I guess the hardware defaults to top right and BGR?

	// NOTE: If the display clears "top to bottom" then 0, 0 defaults to the top right. This is why
	// column address order below is swapped.

	u8 rowColExchangeBit = 5;

	// Vertical
	#if true
	u8 rowAddressOrder_TopToBottom        = 0 << 7;
	u8 colAddressOrder_LeftToRight        = 1 << 6;
	u8 rowColExchange_Yes                 = 1 << 5;
	u8 verticalRefreshOrder_TopToBottom   = 0 << 4;
	u8 rgbOrder_RGB                       = 1 << 3;
	u8 horizontalRefershOrder_LeftToRight = 0 << 2;

	u8 mac = 0;
	mac |= rowAddressOrder_TopToBottom;
	mac |= colAddressOrder_LeftToRight;
	mac |= rowColExchange_Yes;
	mac |= verticalRefreshOrder_TopToBottom;
	mac |= rgbOrder_RGB;
	mac |= horizontalRefershOrder_LeftToRight;

	// Horizontal
	#else
	u8 rowAddressOrder_TopToBottom        = 0 << 7;
	u8 colAddressOrder_LeftToRight        = 1 << 6;
	u8 rowColExchange_Yes                 = 1 << 5;
	u8 verticalRefreshOrder_TopToBottom   = 0 << 4;
	u8 rgbOrder_BGR                       = 0 << 3;
	u8 horizontalRefershOrder_LeftToRight = 0 << 2;

	u8 mac = 0;
	mac |= rowAddressOrder_TopToBottom;
	mac |= colAddressOrder_LeftToRight;
	mac |= rowColExchange_Yes;
	mac |= verticalRefreshOrder_TopToBottom;
	mac |= rgbOrder_RGB;
	mac |= horizontalRefershOrder_LeftToRight;
	#endif

	ILI9341_Write(ili9341, ILI9341::Command::MemoryAccessControl, mac);

	if (GetBit(mac, rowColExchangeBit) == 0)
	{
		ili9341.rowColSwap = false;
		ili9341.xMax = 240 - 1;
		ili9341.yMax = 320 - 1;
	}
	else
	{
		ili9341.rowColSwap = true;
		ili9341.xMax = 320 - 1;
		ili9341.yMax = 240 - 1;
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
	ILI9341_Write(ili9341, ILI9341::Command::FrameRateControlNormal, frc_divisionRatio, frc_clocksPerLine);
}

void
ILI9341_DisplayFunctionControl(ILI9341State& ili9341)
{
	// NOTE: Display Function Control defaults are fine.
	UNUSED(ili9341);
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

	ILI9341_Write(ili9341, ILI9341::Command::PowerControlB, pcb_p1, pcb_p2);
	// NOTE: Leave parameter 3 at default
	ILI9341_WriteCmd(ili9341, ILI9341::Command::Nop);
}

void
ILI9341_VCOMControl(ILI9341State& ili9341)
{
	// NOTE: VCOM Control 1 defaults are fine.
	// NOTE: VCOM Control 2 defaults are fine.
	UNUSED(ili9341);
}

void
ILI9341_DriverTimingControl(ILI9341State& ili9341)
{
	// NOTE: Driver Timing Control A (1) defaults are fine
	// NOTE: Driver Timing Control A (2) isn't used by any example code

	// NOTE: Just doing what everyone else does...
	ILI9341_Write(ili9341, ILI9341::Command::DriverTimingControlB, 0x00, 0x00);
}

void
ILI9341_PowerOnSequenceControl(ILI9341State& ili9341)
{
	// NOTE: Just doing what everyone else does...
	ILI9341_Write(ili9341, ILI9341::Command::PowerOnSequenceControl, 0x64, 0x03, 0x12, 0x81);
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
	ILI9341_Write(ili9341, ILI9341::Command::PositiveGammaCorrection,
		0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00);
	ILI9341_Write(ili9341, ILI9341::Command::NegativeGammaCorrection,
		0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F);
}

void
ILI9341_SetColumnAddress(ILI9341State& ili9341, u16 xMin, u16 xMax)
{
	ILI9341_Write(ili9341, ILI9341::Command::ColumnAddressSet, UnpackMSB2(xMin), UnpackMSB2(xMax));
}

void
ILI9341_SetRowAddress(ILI9341State& ili9341, u16 yMin, u16 yMax)
{
	ILI9341_Write(ili9341, ILI9341::Command::PageAddressSet, UnpackMSB2(yMin), UnpackMSB2(yMax));
}

void
ILI9341_SetWriteAddress(ILI9341State& ili9341, u16 xMin, u16 xMax, u16 yMin, u16 yMax)
{
	ILI9341_Write(ili9341, ILI9341::Command::ColumnAddressSet, UnpackMSB2(xMin), UnpackMSB2(xMax));
	ILI9341_Write(ili9341, ILI9341::Command::PageAddressSet, UnpackMSB2(yMin), UnpackMSB2(yMax));
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
ILI9341_SetRect(ILI9341State& ili9341, u16 xMin, u16 yMin, u16 xMax, u16 yMax, u16 color)
{
	// NOTE: Each MemoryWrite command resets the page and column register to the previous set value
	// TODO: Maybe we should take advantage of that?

	Assert(xMin <= xMax);
	Assert(xMax <= ili9341.xMax);
	ILI9341_Write(ili9341, ILI9341::Command::ColumnAddressSet, UnpackMSB2(xMin), UnpackMSB2(xMax));
	Assert(yMin <= yMax);
	Assert(yMax <= ili9341.yMax);
	ILI9341_Write(ili9341, ILI9341::Command::PageAddressSet, UnpackMSB2(yMin), UnpackMSB2(yMax));
	ILI9341_WriteCmd(ili9341, ILI9341::Command::MemoryWrite);

	u32 totalDataLen = (u32) (2 * (xMax - xMin + 1) * (yMax - yMin + 1));

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

		ILI9341_WriteData(ili9341, bytes);
		remainingLen -= bytes.length;
	}
}

void
ILI9341_SetPixel(ILI9341State& ili9341, u16 x, u16 y, u16 color)
{
	ILI9341_SetRect(ili9341, x, y, x, y, color);
}

void
ILI9341_DisplayTest(ILI9341State& ili9341)
{
	for (u16 i = 0; i < 120; i++)
	{
		u16 l = (u16) (0 + i);
		u16 r = (u16) (ili9341.xMax - i);
		u16 t = (u16) (0 + i);
		u16 b = (u16) (ili9341.yMax - i);
		ILI9341_SetPixel(ili9341, l, t, Colors16::Red);
		ILI9341_SetPixel(ili9341, r, t, Colors16::Green);
		ILI9341_SetPixel(ili9341, l, b, Colors16::Blue);
		ILI9341_SetPixel(ili9341, r, b, Colors16::Gray);
	}

	u16 xQuarter = (u16) ((ili9341.xMax + 1) / 4);
	u16 yQuarter = (u16) ((ili9341.yMax + 1) / 4);

	if (ili9341.rowColSwap)
	{
		for (u16 i = 0; i < 80; i++)
		{
			u16 m = (u16) (120 + i);
			u16 t = (u16) (2 * yQuarter + 0);
			u16 b = (u16) (2 * yQuarter + 1);
			ILI9341_SetPixel(ili9341, m, t, Colors16::Black);
			ILI9341_SetPixel(ili9341, m, b, Colors16::Black);
		}
	}
	else
	{
		for (u16 i = 0; i < 80; i++)
		{
			u16 l = (u16) (2 * xQuarter + 0);
			u16 r = (u16) (2 * xQuarter + 1);
			u16 m = (u16) (120 + i);
			ILI9341_SetPixel(ili9341, l, m, Colors16::Black);
			ILI9341_SetPixel(ili9341, r, m, Colors16::Black);
		}
	}

	ILI9341_SetRect(ili9341, (u16) (2 * xQuarter - 10), (u16) (1 * yQuarter - 10), (u16) (2 * xQuarter + 10), (u16) (1 * yQuarter + 10), Colors16::Yellow);  // T
	ILI9341_SetRect(ili9341, (u16) (1 * xQuarter - 10), (u16) (2 * yQuarter - 10), (u16) (1 * xQuarter + 10), (u16) (2 * yQuarter + 10), Colors16::Magenta); // L
	ILI9341_SetRect(ili9341, (u16) (3 * xQuarter - 10), (u16) (2 * yQuarter - 10), (u16) (3 * xQuarter + 10), (u16) (2 * yQuarter + 10), Colors16::Gray);    // R
	ILI9341_SetRect(ili9341, (u16) (2 * xQuarter - 10), (u16) (3 * yQuarter - 10), (u16) (2 * xQuarter + 10), (u16) (3 * yQuarter + 10), Colors16::Cyan);    // B
}

void
ILI9341_Clear(ILI9341State& ili9341, u16 color)
{
	ILI9341_SetRect(ili9341, 0, 0, ili9341.xMax, ili9341.yMax, color);
}

void
ILI9341_BeginDrawFrames(ILI9341State& ili9341)
{
	Assert(!ili9341.drawingFrames);
	ili9341.drawingFrames = true;

	u16 xMin = 0;
	u16 yMin = 0;
	ILI9341_Write(ili9341, ILI9341::Command::ColumnAddressSet, UnpackMSB2(xMin), UnpackMSB2(ili9341.xMax));
	ILI9341_Write(ili9341, ILI9341::Command::PageAddressSet,   UnpackMSB2(yMin), UnpackMSB2(ili9341.yMax));
	ILI9341_WriteCmd(ili9341, ILI9341::Command::MemoryWrite);
}

void
ILI9341_EndDrawFrames(ILI9341State& ili9341)
{
	Assert(ili9341.drawingFrames);
	ili9341.drawingFrames = false;
}

void
ILI9341_DrawFrame(ILI9341State& ili9341, ByteSlice bytes)
{
	Assert(ili9341.drawingFrames);
	Assert(bytes.stride == 1);

	// TODO: Remove endian swap when using parallel interface
	Assert(bytes.length % 2 == 0);
	for (u32 i = 0; i < bytes.length; i += 2)
	{
		u8 temp = bytes[i];
		bytes[i + 0] = bytes[i + 1];
		bytes[i + 1] = temp;
	}

	ILI9341_WriteData(ili9341, bytes);
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

b8
ILI9341_Initialize(ILI9341State& ili9341, FT232HState& ft232h)
{
	ili9341.ft232h = &ft232h;

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

	return true;
}

void
ILI9341_Teardown(ILI9341State&) {}
