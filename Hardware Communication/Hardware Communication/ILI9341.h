namespace ILI9341
{
	struct Command
	{
		static const u8 ColumnAddressSet = 0x2A;
		static const u8 PageAddressSet   = 0x2B;
		static const u8 MemoryWrite      = 0x2C;
	};
}

void ILI9341_WriteCmd(FT232H::State* ft232h, u8 cmd)
{
	FT232H_SetDC(ft232h, Signal::Low);

	// NOTE: LCD reads on the rising edge so write on the falling edge
	u16 ftcmdSize = 1;
	Trace("cmd 0x%.2X\n", cmd);
	u8 ftcmd[] = { FT232H::Command::WRITE_BYTES_NVE_MSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
	FT232H_Write(ft232h, ftcmd);
	FT232H_Write(ft232h, cmd);
}

void ILI9341_WriteData(FT232H::State* ft232h, u8* data, u16 dataLen)
{
	FT232H_SetDC(ft232h, Signal::High);

	u16 ftcmdSize = dataLen;
#if ENABLE_TRACE
	Trace("write data");
	for (int i = 0; i < dataLen; i++) Trace(" 0x%.2X", data[i]);
	Trace("\n");
#endif
	// NOTE: LCD reads on the rising edge so write on the falling edge
	u8 ftcmd[] = { FT232H::Command::WRITE_BYTES_NVE_MSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
	FT232H_Write(ft232h, ftcmd);
	FT232H_Write(ft232h, data, dataLen);
}

template <u16 N>
void ILI9341_WriteData(FT232H::State* ft232h, u8 (&data)[N])
{
	ILI9341_WriteData(ft232h, data, N);
}

template <typename ...Args>
void ILI9341_WriteData(FT232H::State* ft232h, Args... bytes)
{
	u8 data[] = { u8(bytes)... };
	ILI9341_WriteData(ft232h, data);
}

template <u16 N>
void ILI9341_Write(FT232H::State* ft232h, u8 cmd, u8 (&data)[N])
{
	ILI9341_WriteCmd(ft232h, cmd);
	ILI9341_WriteData(ft232h, data);
}

template <typename ...Args>
void ILI9341_Write(FT232H::State* ft232h, u8 cmd, Args... bytes)
{
	ILI9341_WriteCmd(ft232h, cmd);
	ILI9341_WriteData(ft232h, bytes...);
}

void ILI9341_SetRect(FT232H::State* ft232h, u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	Assert(x1 <= x2);
	Assert(x2 <= 240 - 1);
	//Assert(MADCTL == 0 && x <= 0x00EF);
	//Assert(MADCTL == 1 && x <= 0x013F);
	ILI9341_Write(ft232h, ILI9341::Command::ColumnAddressSet, Unpack2(x1), Unpack2(x2));
	Assert(y1 <= y2);
	Assert(y2 <= 320 - 1);
	//Assert(MADCTL == 0 && y <= 0x013F);
	//Assert(MADCTL == 1 && y <= 0x00EF);
	ILI9341_Write(ft232h, ILI9341::Command::PageAddressSet, Unpack2(y1), Unpack2(y2));

	// TODO: Change rects to be exclusive of max?
	// HACK: lol allocation
	u16 dataLen = 2 * (x2 - x1 + 1) * (y2 - y1 + 1);
	u8* colorData = new u8[dataLen];
	for (u16 i = 0; i < dataLen; i += 2)
	{
		colorData[i + 0] = BYTE(1, color);
		colorData[i + 1] = BYTE(0, color);
	}
	ILI9341_Write(ft232h, ILI9341::Command::MemoryWrite, colorData, dataLen);
	delete[] colorData;
}

void ILI9341_SetPixel(FT232H::State* ft232h, u16 x, u16 y, u16 color)
{
	ILI9341_SetRect(ft232h, x, y, x, y, color);
}

void DocumentationInit(FT232H::State* ft232h)
{
	// NOTE: I'm not sure why write_data16 passes two bytes. The first is always zero and is not part
	// of the parameters.

	auto delay        = [](DWORD ms) { Sleep(ms); };
	auto write_cmd    = [&](u8 cmd) { ILI9341_WriteCmd(ft232h, cmd); };
	auto write_data16 = [&](u8 byte1, u8 byte2) { ILI9341_WriteData(ft232h, byte2); };

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

void PythonInit(FT232H::State* ft232h)
{
	// Pixel Format 0x55 = 0b'0101'0101 = 16bpp RGB and MCU

	ILI9341_Write(ft232h, 0xEF, 0x03, 0x80, 0x02);             //* Can't find command!
	ILI9341_Write(ft232h, 0xCF, 0x00, 0xC1, 0x30);             //* Power Control B                    values
	ILI9341_Write(ft232h, 0xED, 0x64, 0x03, 0x12, 0x81);       // Power On Sequence Control
	ILI9341_Write(ft232h, 0xE8, 0x85, 0x00, 0x78);             //* Driver Timing Control A            1 extra 0
	ILI9341_Write(ft232h, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02); // Power Control A
	ILI9341_Write(ft232h, 0xF7, 0x20);                         // Pump Ratio Control
	ILI9341_Write(ft232h, 0xEA, 0x00, 0x00);                   // Driver Control Timing B
	ILI9341_Write(ft232h, 0xC0, 0x23);                         //* Power Control 1, VRH[5:0]          value
	ILI9341_Write(ft232h, 0xC1, 0x10);                         //* Power Control 2, SAP[2:0], BT[3:0] value
	ILI9341_Write(ft232h, 0xC5, 0x3E, 0x28);                   //* VCM Control 1                      values
	ILI9341_Write(ft232h, 0xC7, 0x86);                         //* VCM Control 2                      value
	ILI9341_Write(ft232h, 0x36, 0x48);                         // Memory Access Control
	ILI9341_Write(ft232h, 0x3A, 0x55);                         // Pixel Format
	ILI9341_Write(ft232h, 0xB1, 0x00, 0x18);                   // FRMCTR1                             values
	ILI9341_Write(ft232h, 0xB6, 0x08, 0x82, 0x27);             //* Display Function Control           value  order
	ILI9341_Write(ft232h, 0xF2, 0x00);                         //* 3Gamma Function Disable            value
	ILI9341_Write(ft232h, 0x26, 0x01);                         // Gamma Curve Selected
	ILI9341_Write(ft232h, 0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00); // Set Gamma
	ILI9341_Write(ft232h, 0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F); // Set Gamma
	// column
	// page
	// entry mode set
	ILI9341_WriteCmd(ft232h, 0x11);                            // Sleep Out
	ILI9341_WriteCmd(ft232h, 0x29);                            // Display On
	// memory write
}

void ILI9341_Initialize(FT232H::State* ft232h)
{
	//DocumentationInit(ft232h);
	PythonInit(ft232h);
}

void ILI9341_Finalize(FT232H::State* ft232h) {}
