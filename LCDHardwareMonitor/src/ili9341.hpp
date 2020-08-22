struct ILI9341State
{
	FT232HState* ft232h;

	b8  sleep;
	i64 sleepTime;
	i64 wakeTime;
	b8  rowColSwap;
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

static inline u8
GetByte(u8 index, u32 value) { return (value >> (index * 8)) & 0xFF; }

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
	//ili9341->nextCommandTime = ili9341->sleepTime + 5ms
}

// TODO: Revisit this when displaying LHM frames.
void
ILI9341_MemoryAccessControl(ILI9341State& ili9341)
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
	ili9341.rowColSwap = false;
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

	u8 params[] = { pcb_p1, pcb_p2 };
	ILI9341_Write(ili9341, ILI9341::Command::PowerControlB, params);
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
	u8 posParams[] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
	ILI9341_Write(ili9341, ILI9341::Command::PositiveGammaCorrection, posParams);
	u8 negParams[] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
	ILI9341_Write(ili9341, ILI9341::Command::NegativeGammaCorrection, negParams);
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

	return true;
}

void
ILI9341_Teardown(ILI9341State&) {}
