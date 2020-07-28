namespace ILI9341
{
	struct State
	{
		FT232H::State* ft232h;
		bool sleep;
		u64 sleepTime;
		u64 wakeTime;

		//MADCTL_B5
		//Column
		//Page
		//RamWrite
		//nextCommandTime
	};

	struct Command
	{
		static const u8 SoftwareReset    = 0x01;
		static const u8 SleepIn          = 0x10;
		static const u8 SleepOut         = 0x11;
		static const u8 ColumnAddressSet = 0x2A;
		static const u8 PageAddressSet   = 0x2B;
		static const u8 MemoryWrite      = 0x2C;
	};
}

void ILI9341_WriteCmd(ILI9341::State* ili9341, u8 cmd)
{
	FT232H_SetDC(ili9341->ft232h, Signal::Low);

	// NOTE: LCD reads on the rising edge so write on the falling edge
	u16 ftcmdSize = 1;
	Trace("cmd 0x%.2X\n", cmd);
	u8 ftcmd[] = { FT232H::Command::WRITE_BYTES_NVE_MSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
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
	u8 ftcmd[] = { FT232H::Command::WRITE_BYTES_NVE_MSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
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

// TODO: Change rects to be exclusive of max?
void ILI9341_SetRect(ILI9341::State* ili9341, u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	Assert(x1 <= x2);
	Assert(x2 <= 240 - 1);
	//Assert(MADCTL == 0 && x <= 0x00EF);
	//Assert(MADCTL == 1 && x <= 0x013F);
	ILI9341_Write(ili9341, ILI9341::Command::ColumnAddressSet, Unpack2(x1), Unpack2(x2));
	Assert(y1 <= y2);
	Assert(y2 <= 320 - 1);
	//Assert(MADCTL == 0 && y <= 0x013F);
	//Assert(MADCTL == 1 && y <= 0x00EF);
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
	//nextCommandTime = sleepTime + 5ms
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
	//nextCommandTime = sleepTime + 5ms;
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
	//nextCommandTime = sleepTime + 5ms
}

void DocumentationInit(ILI9341::State* ili9341)
{
	// NOTE: I'm not sure why write_data16 passes two bytes. The first is always zero and is not part
	// of the parameters.

	auto delay        = [](DWORD ms) { Sleep(ms); };
	auto write_cmd    = [&](u8 cmd) { ILI9341_WriteCmd(ili9341, cmd); };
	auto write_data16 = [&](u8 byte1, u8 byte2) { ILI9341_WriteData(ili9341, byte2); };

	write_cmd(0x01); // software reset
	delay(5);

	write_cmd(0x28); // display off

	//---------------------------------------------------------

	write_cmd(0xCF); // power control B
	write_data16(0x00, 0x00);
	write_data16(0x00, 0x83);
	write_data16(0x00, 0x30);

	write_cmd(0xED); // power on sequence control
	write_data16(0x00, 0x64);
	write_data16(0x00, 0x03);
	write_data16(0x00, 0x12);
	write_data16(0x00, 0x81);

	write_cmd(0xE8); // driver timing control A
	write_data16(0x00, 0x85);
	write_data16(0x00, 0x01);
	write_data16(0x00, 0x79);

	write_cmd(0xCB); // power control A
	write_data16(0x00, 0x39);
	write_data16(0x00, 0x2C);
	write_data16(0x00, 0x00);
	write_data16(0x00, 0x34);
	write_data16(0x00, 0x02);

	write_cmd(0xF7); // pump ratio control
	write_data16(0x00, 0x20);

	write_cmd(0xEA); // driver timing control B
	write_data16(0x00, 0x00);
	write_data16(0x00, 0x00);

	//------------power control------------------------------

	write_cmd(0xC0); // power control 1
	write_data16(0x00, 0x26);

	write_cmd(0xC1); // power control 2
	write_data16(0x00, 0x11);

	//--------------VCOM---------

	write_cmd(0xC5); // vcom control 1
	write_data16(0x00, 0x35);
	write_data16(0x00, 0x3E);

	write_cmd(0xC7); // vcom control 2
	write_data16(0x00, 0xBE); // 0x94

	//------------memory access control------------------------

	write_cmd(0x36); // memory access control
	write_data16(0x00, 0x48); // my, mx, mv, ml, BGR, mh, 0.0

	write_cmd(0x3A); // pixel format set
	write_data16(0x00, 0x55); // 16bit / pixel

	//-----------------  frame  rate------------------------------

	write_cmd(0xB1); // frame control (normal mode)
	write_data16(0x00, 0x00);
	write_data16(0x00, 0x1B); // 70

	//----------------Gamma---------------------------------

	write_cmd(0xF2); // 3Gamma Function Disable
	write_data16(0x00, 0x08);

	write_cmd(0x26); // gamma set
	write_data16(0x00, 0x01); // gamma set 4 gamma curve 01/02/04/08

	write_cmd(0xE0); // positive gamma correction
	write_data16(0x00, 0x1F);
	write_data16(0x00, 0x1A);
	write_data16(0x00, 0x18);
	write_data16(0x00, 0x0A);
	write_data16(0x00, 0x0F);
	write_data16(0x00, 0x06);
	write_data16(0x00, 0x45);
	write_data16(0x00, 0x87);
	write_data16(0x00, 0x32);
	write_data16(0x00, 0x0A);
	write_data16(0x00, 0x07);
	write_data16(0x00, 0x02);
	write_data16(0x00, 0x07);
	write_data16(0x00, 0x05);
	write_data16(0x00, 0x00);

	write_cmd(0xE1); // negative gamma correction
	write_data16(0x00, 0x00);
	write_data16(0x00, 0x25);
	write_data16(0x00, 0x27);
	write_data16(0x00, 0x05);
	write_data16(0x00, 0x10);
	write_data16(0x00, 0x09);
	write_data16(0x00, 0x3A);
	write_data16(0x00, 0x78);
	write_data16(0x00, 0x4D);
	write_data16(0x00, 0x05);
	write_data16(0x00, 0x18);
	write_data16(0x00, 0x0D);
	write_data16(0x00, 0x38);
	write_data16(0x00, 0x3A);
	write_data16(0x00, 0x1F);

	//--------------ddram ---------------------

	write_cmd(0x2A); // column address set
	write_data16(0x00, 0x00);
	write_data16(0x00, 0x00);
	write_data16(0x00, 0x00);
	write_data16(0x00, 0xEF);

	write_cmd(0x2B); // page address set
	write_data16(0x00, 0x00);
	write_data16(0x00, 0x00);
	write_data16(0x00, 0x01);
	write_data16(0x00, 0x3F);

	//write_cmd(0x34); // tearing effect line off
	//write_cmd(0x35); // tearing effect line on

	//write_cmd(0xB4); // display inversion control
	//write_data16(0x00, 0x00);

	write_cmd(0xB7); // entry mode set
	write_data16(0x00, 0x07);

	//-----------------display---------------------

	write_cmd(0xB6); // display function control
	write_data16(0x00, 0x0A);
	write_data16(0x00, 0x82);
	write_data16(0x00, 0x27);
	write_data16(0x00, 0x00);

	write_cmd(0x11); // sleep out
	delay(100);
	write_cmd(0x29); // display on
	delay(100);
	write_cmd(0x2C); // memory write
}

void PythonInit(ILI9341::State* ili9341)
{
	// Pixel Format 0x55 = 0b'0101'0101 = 16bpp RGB and MCU

	ILI9341_Write(ili9341, 0xEF, 0x03, 0x80, 0x02);             //* Can't find command!
	ILI9341_Write(ili9341, 0xCF, 0x00, 0xC1, 0x30);             //* Power Control B                    values
	ILI9341_Write(ili9341, 0xED, 0x64, 0x03, 0x12, 0x81);       // Power On Sequence Control
	ILI9341_Write(ili9341, 0xE8, 0x85, 0x00, 0x78);             //* Driver Timing Control A            1 extra 0
	ILI9341_Write(ili9341, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02); // Power Control A
	ILI9341_Write(ili9341, 0xF7, 0x20);                         // Pump Ratio Control
	ILI9341_Write(ili9341, 0xEA, 0x00, 0x00);                   // Driver Control Timing B
	ILI9341_Write(ili9341, 0xC0, 0x23);                         //* Power Control 1, VRH[5:0]          value
	ILI9341_Write(ili9341, 0xC1, 0x10);                         //* Power Control 2, SAP[2:0], BT[3:0] value
	ILI9341_Write(ili9341, 0xC5, 0x3E, 0x28);                   //* VCM Control 1                      values
	ILI9341_Write(ili9341, 0xC7, 0x86);                         //* VCM Control 2                      value
	ILI9341_Write(ili9341, 0x36, 0x48);                         // Memory Access Control
	ILI9341_Write(ili9341, 0x3A, 0x55);                         // Pixel Format
	ILI9341_Write(ili9341, 0xB1, 0x00, 0x18);                   // FRMCTR1                             values
	ILI9341_Write(ili9341, 0xB6, 0x08, 0x82, 0x27);             //* Display Function Control           value  order
	ILI9341_Write(ili9341, 0xF2, 0x00);                         //* 3Gamma Function Disable            value
	ILI9341_Write(ili9341, 0x26, 0x01);                         // Gamma Curve Selected
	ILI9341_Write(ili9341, 0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00); // Set Gamma
	ILI9341_Write(ili9341, 0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F); // Set Gamma
	// column
	// page
	// entry mode set
	ILI9341_WriteCmd(ili9341, 0x11);                            // Sleep Out
	ILI9341_WriteCmd(ili9341, 0x29);                            // Display On
	// memory write
}

void ArdnewInit(ILI9341::State* ili9341)
{
	// Hardware reset
	ILI9341_WriteCmd(ili9341, 0x01);                            // software reset
	Sleep(200);
	ILI9341_Write(ili9341, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02); // power control A
	ILI9341_Write(ili9341, 0xCF, 0x00, 0xC1, 0x30);             // power control B
	ILI9341_Write(ili9341, 0xE8, 0x85, 0x00, 0x78);             // driver timing control A
	ILI9341_Write(ili9341, 0xEA, 0x00, 0x00);                   // driver timing control B
	ILI9341_Write(ili9341, 0xED, 0x64, 0x03, 0x12, 0x81);       // power-on sequence control
	ILI9341_Write(ili9341, 0xF7, 0x20);                         // pump ratio control
	ILI9341_Write(ili9341, 0xC0, 0x23);                         // power control VRH[5:0]
	ILI9341_Write(ili9341, 0xC1, 0x10);                         // power control SAP[2:0] BT[3:0]
	ILI9341_Write(ili9341, 0xC5, 0x3E, 0x28);                   // VCM control
	ILI9341_Write(ili9341, 0xC7, 0x86);                         // VCM control 2
	ILI9341_Write(ili9341, 0x36, 0x48);                         // memory access control
	ILI9341_Write(ili9341, 0x3A, 0x55);                         // pixel format
	ILI9341_Write(ili9341, 0xB1, 0x00, 0x18);                   // frame ratio control, standard RGB color
	ILI9341_Write(ili9341, 0xB6, 0x08, 0x82, 0x27);             // display function control
	ILI9341_Write(ili9341, 0xF2, 0x00);                         // 3-Gamma function disable
	ILI9341_Write(ili9341, 0x26, 0x01);                         // gamma curve selected
	ILI9341_Write(ili9341, 0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00); // positive gamma correction
	ILI9341_Write(ili9341, 0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F); // negative gamma correction
	ILI9341_WriteCmd(ili9341, 0x11);                            // exit sleep
	Sleep(120);
	ILI9341_WriteCmd(ili9341, 0x29);                            // turn on display
	//ILI9341_Write(ili9341, 0x36, []uint8{lcd.config.Rotate.MADCTL()); // MADCTL
}

void ILI9341_Initialize(ILI9341::State* ili9341, FT232H::State* ft232h)
{
	ili9341->ft232h = ft232h;

	//DocumentationInit(ili9341);
	PythonInit(ili9341);
	//ArdnewInit(ili9341);
}

void ILI9341_Finalize(ILI9341::State* ili9341) {}
