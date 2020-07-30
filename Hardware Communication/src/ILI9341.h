namespace ILI9341
{
	struct State
	{
		FT232H::State* ft232h;
		bool sleep;
		u64 sleepTime;
		u64 wakeTime;
		//u64 nextCommandTime;
		bool rowColSwap;

		//Column
		//Page
		//RamWrite
	};

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

void ILI9341_WriteCmd(ILI9341::State* ili9341, u8 cmd)
{
	FT232H_SetDC(ili9341->ft232h, Signal::Low);

	// NOTE: LCD reads on the rising edge so write on the falling edge
	u16 ftcmdSize = 1;
	Trace("cmd 0x%.2X\n", cmd);
	u8 ftcmd[] = { FT232H::Command::SendBytesFallingMSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
	FT232H_Write(ili9341->ft232h, ftcmd);
	FT232H_Write(ili9341->ft232h, cmd);
}

void ILI9341_WriteData(ILI9341::State* ili9341, u8* data, u16 dataLen)
{
	FT232H_SetDC(ili9341->ft232h, Signal::High);

	u16 ftcmdSize = dataLen;
#if ENABLE_TRACE
	Trace("write data");
	for (int i = 0; i < dataLen; i++) Trace(" 0x%.2X", data[i]);
	Trace("\n");
#endif
	// NOTE: LCD reads on the rising edge so write on the falling edge
	u8 ftcmd[] = { FT232H::Command::SendBytesFallingMSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
	FT232H_Write(ili9341->ft232h, ftcmd);
	FT232H_Write(ili9341->ft232h, data, dataLen);
}

template <u16 N>
void ILI9341_WriteData(ILI9341::State* ili9341, u8 (&data)[N])
{
	ILI9341_WriteData(ili9341, data, N);
}

template <typename ...Args>
void ILI9341_WriteData(ILI9341::State* ili9341, Args... bytes)
{
	u8 data[] = { u8(bytes)... };
	ILI9341_WriteData(ili9341, data);
}

template <u16 N>
void ILI9341_Write(ILI9341::State* ili9341, u8 cmd, u8 (&data)[N])
{
	ILI9341_WriteCmd(ili9341, cmd);
	ILI9341_WriteData(ili9341, data);
}

template <typename ...Args>
void ILI9341_Write(ILI9341::State* ili9341, u8 cmd, Args... bytes)
{
	ILI9341_WriteCmd(ili9341, cmd);
	ILI9341_WriteData(ili9341, bytes...);
}

// TODO: Something about this is wrong. If this is the only command sent back to back runs result in
// a non-empty read buffer
// TODO: Change rects to be exclusive of max?
void ILI9341_SetRect(ILI9341::State* ili9341, u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	Assert(x1 <= x2);
	Assert(ili9341->rowColSwap ? x2 < 320 : x2 < 240);
	ILI9341_Write(ili9341, ILI9341::Command::ColumnAddressSet, Unpack2(x1), Unpack2(x2));
	Assert(y1 <= y2);
	Assert(ili9341->rowColSwap ? y2 < 240 : y2 < 320);
	ILI9341_Write(ili9341, ILI9341::Command::PageAddressSet, Unpack2(y1), Unpack2(y2));

	const u32 maxWrite = 0xFFFE;
	u8 colorData[maxWrite];
	u32 dataLen = 2 * (x2 - x1 + 1) * (y2 - y1 + 1);
	for (u32 i = 0; i < Min(dataLen, maxWrite); i += 2)
	{
		colorData[i + 0] = BYTE(1, color);
		colorData[i + 1] = BYTE(0, color);
	}

	ILI9341_WriteCmd(ili9341, ILI9341::Command::MemoryWrite);

	u32 remainingLen = dataLen;
	while (remainingLen > 0)
	{
		u16 writeLen = Min(remainingLen, maxWrite);
		ILI9341_WriteData(ili9341, colorData, writeLen);
		remainingLen -= writeLen;
	}
}

void ILI9341_SetPixel(ILI9341::State* ili9341, u16 x, u16 y, u16 color)
{
	ILI9341_SetRect(ili9341, x, y, x, y, color);
}

void ILI9341_Clear(ILI9341::State* ili9341, u16 color)
{
	ILI9341_SetRect(ili9341, 0, 0, 240 - 1, 320 - 1, color);
}

void ILI9341_Reset(ILI9341::State* ili9341)
{
	//NOTE: Resets device state to defaults
	ILI9341_WriteCmd(ili9341, ILI9341::Command::SoftwareReset);
	ili9341->sleep = true;
	ili9341->sleepTime = GetTime();
	Sleep(5);
	//ili9341->nextCommandTime = ili9341->sleepTime + 5ms
}

void ILI9341_Sleep(ILI9341::State* ili9341)
{
	// NOTE: MCU are memory are still working. Memory maintains contents.
	Assert(!ili9341->sleep);

	u32 sinceWake = u32(GetElapsedMs(ili9341->wakeTime));
	if (sinceWake < 120)
			Sleep(120 - sinceWake);

	ILI9341_WriteCmd(ili9341, ILI9341::Command::SleepIn);
	ili9341->sleep = true;
	ili9341->sleepTime = GetTime();
	Sleep(5);
	//ili9341->nextCommandTime = ili9341->sleepTime + 5ms;
}

void ILI9341_Wake(ILI9341::State* ili9341)
{
	Assert(ili9341->sleep);

	u32 sinceSleep = u32(GetElapsedMs(ili9341->sleepTime));
	if (sinceSleep < 120)
		Sleep(120 - sinceSleep);

	ILI9341_WriteCmd(ili9341, ILI9341::Command::SleepOut);
	ili9341->sleep = false;
	ili9341->wakeTime = GetTime();
	// NOTE: Docs contradict themselves. Is it 5 ms or 120 ms?
	Sleep(5);
	//ili9341->nextCommandTime = ili9341->wakeTime + 5ms
}

// TODO: Revisit this when displaying LHM frames.
void ILI9341_MemoryAccessControl(ILI9341::State* ili9341)
{
	// NOTE: I have no idea why bits 3 and 6 have to be flipped, but every other piece of code I can
	// find does the same, including an example program from the manufacturer of one of these
	// screens. Despite the documentation, I guess the hardware defaults to top right and BGR?

	u8 rowAddressOrder_TopToBottom        = 0 << 7;
	u8 colAddressOrder_LeftToRight        = 1 << 6;
	u8 rowColExchange_No                  = 0 << 5;
	u8 berticalRefreshOrder_TopToBottom   = 0 << 4;
	u8 rgbOrder_RGB                       = 1 << 3;
	u8 horizontalRefershOrder_LeftToRight = 0 << 2;

	u8 mac = 0;
	mac |= rowAddressOrder_TopToBottom;
	mac |= colAddressOrder_LeftToRight;
	mac |= rowColExchange_No;
	mac |= berticalRefreshOrder_TopToBottom;
	mac |= rgbOrder_RGB;
	mac |= horizontalRefershOrder_LeftToRight;
	ILI9341_Write(ili9341, ILI9341::Command::MemoryAccessControl, mac);
	ili9341->rowColSwap = false;
}

void ILI9341_PixelFormatSet(ILI9341::State* ili9341)
{
	u8 bpp16 = 0b101;
	u8 bpp18 = 0b110;

	u8 mcuShift = 0;
	u8 rgbShift = 4;

	u8 pf = 0;
	pf |= bpp16 << mcuShift;
	pf |= bpp16 << rgbShift;

	ILI9341_Write(ili9341, ILI9341::Command::PixelFormatSet, pf);
}

void ILI9341_FrameRateControl_Normal(ILI9341::State* ili9341)
{
	u8 divisionRatio_1 = 0b00;
	u8 divisionRatio_2 = 0b01;
	u8 divisionRatio_4 = 0b10;
	u8 divisionRatio_8 = 0b11;

	u8 frameRate_119 = 0b10000;
	u8 frameRate_112 = 0b10001;
	u8 frameRate_106 = 0b10010;
	u8 frameRate_100 = 0b10011;
	u8 frameRate_95  = 0b10100;
	u8 frameRate_90  = 0b10101;
	u8 frameRate_86  = 0b10110;
	u8 frameRate_83  = 0b10111;
	u8 frameRate_79  = 0b11000;
	u8 frameRate_76  = 0b11001;
	u8 frameRate_73  = 0b11010;
	u8 frameRate_70  = 0b11011;
	u8 frameRate_68  = 0b11100;
	u8 frameRate_65  = 0b11101;
	u8 frameRate_63  = 0b11110;
	u8 frameRate_61  = 0b11111;

	u8 frc_divisionRatio = divisionRatio_1;
	u8 frc_clocksPerLine = frameRate_119;
	ILI9341_Write(ili9341, ILI9341::Command::FrameRateControlNormal, frc_divisionRatio, frc_clocksPerLine);
}

void ILI9341_DisplayFunctionControl(ILI9341::State* ili9341)
{
	// NOTE: Display Function Control defaults are fine.
}

void ILI9341_PowerControl(ILI9341::State* ili9341)
{
	// NOTE: Power Control 1 defaults are fine.
	// NOTE: Power Control 2 defaults are fine.
	// NOTE: Power Control A defaults are fine.

	u8 pcb_p1_default = 0x00;
	u8 pcb_p2_default = 0x81;
	u8 pcb_p2_pceq_enable = 1 << 6;

	u8 pcb_p1 = pcb_p1_default;
	u8 pcb_p2 = pcb_p2_default | pcb_p2_pceq_enable;

	ILI9341_Write(ili9341, ILI9341::Command::PowerControlB, pcb_p1, pcb_p2);
	// NOTE: Leave parameter 3 at default
	ILI9341_WriteCmd(ili9341, ILI9341::Command::Nop);
}

void ILI9341_VCOMControl(ILI9341::State* ili9341)
{
	// NOTE: VCOM Control 1 defaults are fine.
	// NOTE: VCOM Control 2 defaults are fine.
}

void ILI9341_DriverTimingControl(ILI9341::State* ili9341)
{
	// NOTE: Driver Timing Control A (1) defaults are fine
	// NOTE: Driver Timing Control A (2) isn't used by any example code

	// NOTE: Just doing what everyone else does...
	ILI9341_Write(ili9341, ILI9341::Command::DriverTimingControlB, 0x00, 0x00);
}

void ILI9341_PowerOnSequenceControl(ILI9341::State* ili9341)
{
	// NOTE: Just doing what everyone else does...
	ILI9341_Write(ili9341, ILI9341::Command::PowerOnSequenceControl, 0x64, 0x03, 0x12, 0x81);
}

void ILI9341_SetGamma(ILI9341::State* ili9341)
{
	// NOTE: Enable 3 Gamma default is fine
	// NOTE: Gamma Set default is fine

	// NOTE: Just doing what everyone else does...
	ILI9341_Write(ili9341, ILI9341::Command::PositiveGammaCorrection,
		0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00);
	ILI9341_Write(ili9341, ILI9341::Command::NegativeGammaCorrection,
		0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F);
}

void ILI9341_PumpRatioControl(ILI9341::State* ili9341)
{
	u8 ratioControl_2x = 0b10 << 4;

	u8 prc_p1 = 0;
	prc_p1 |= ratioControl_2x;
	ILI9341_Write(ili9341, ILI9341::Command::PumpRatioControl, prc_p1);
}

void ILI9341_Initialize(ILI9341::State* ili9341, FT232H::State* ft232h)
{
	ili9341->ft232h = ft232h;

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
}

void ILI9341_Finalize(ILI9341::State* ili9341) {}
