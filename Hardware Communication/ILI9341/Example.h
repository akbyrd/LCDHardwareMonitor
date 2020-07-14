void RunExample(FT_HANDLE device)
{
	auto delay = [](DWORD ms)
	{
		Sleep(ms);
	};

	auto write_cmd = [&](u8 cmd)
	{
		u8 pinValues = 0b0000'0000;
		u8 pinDirections = 0b0000'0011;
		u8 pinInitCmd[] = { Command::SetDataBitsHighByte, pinValues, pinDirections };
		Write(device, pinInitCmd);

		u16 ftcmdSize = 1;
		u8 ftcmd[] = { Command::WRITE_BYTES_NVE_MSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
		Write(device, ftcmd);
		Write(device, cmd);
	};

	auto write_data16 = [&](u8 byte1, u8 byte2)
	{
		u8 pinValues = 0b0000'0010;
		u8 pinDirections = 0b0000'0011;
		u8 pinInitData[] = { Command::SetDataBitsHighByte, pinValues, pinDirections };
		Write(device, pinInitData);

		u16 ftcmdSize = 2;
		u8 ftcmd2[] = { Command::WRITE_BYTES_NVE_MSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
		Write(device, ftcmd2);
		u8 data[] = { byte1, byte2 };
		Write(device, data);
	};

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

void PythonInitCopy(FT_HANDLE device)
{
	auto write_cmd = [](FT_HANDLE device, u8 cmd) {
		printf("dc 0\n");
		u8 pinValues = 0b0000'0000;
		u8 pinDirections = 0b0000'0011;
		u8 pinInitCmd[] = { Command::SetDataBitsHighByte, pinValues, pinDirections };
		Write(device, pinInitCmd);

		printf("cmd 0x%.2X\n", cmd);
		u16 ftcmdSize = 1;
		u8 ftcmd[] = { Command::WRITE_BYTES_NVE_MSB, BYTE(0, ftcmdSize - 1), BYTE(1, ftcmdSize - 1) };
		Write(device, ftcmd);
		Write(device, cmd);
	};

	auto write = [](FT_HANDLE device, u8 cmd, auto... data) {
		u8 ar[] = { u8(data)... };
		WriteToScreen(device, cmd, ar);
	};

	write(device, 0xEF, 0x03, 0x80, 0x02);             //? Can't find command!
	write(device, 0xCF, 0x00, 0xC1, 0x30);             //? Power Control B
	write(device, 0xED, 0x64, 0x03, 0x12, 0x81);       //? Power On Sequence Control
	write(device, 0xE8, 0x85, 0x00, 0x78);             //? Driver Timing Control A
	write(device, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02); //? Power Control A
	write(device, 0xF7, 0x20);                         //? Pump Ratio Control
	write(device, 0xEA, 0x00, 0x00);                   //? Driver Control Timing B
	write(device, 0xC0, 0x23);                         // Power Control 1, VRH[5:0]
	write(device, 0xC1, 0x10);                         // Power Control 2, SAP[2:0], BT[3:0]
	write(device, 0xC5, 0x3E, 0x28);                   // VCM Control 1
	write(device, 0xC7, 0x86);                         // VCM Control 2
	write(device, 0x36, 0x48);                         // Memory Access Control
	write(device, 0x3A, 0x55);                         // Pixel Format
	write(device, 0xB1, 0x00, 0x18);                   // FRMCTR1
	write(device, 0xB6, 0x08, 0x82, 0x27);             // Display Function Control
	write(device, 0xF2, 0x00);                         // 3Gamma Function Disable
	write(device, 0x26, 0x01);                         // Gamma Curve Selected
	write(device, 0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00); // Set Gamma
	write(device, 0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F); // Set Gamma
	write_cmd(device, 0x11);                               //? Sleep Out
	write_cmd(device, 0x29);                               //? Display On
}
