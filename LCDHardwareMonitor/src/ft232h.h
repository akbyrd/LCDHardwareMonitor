struct FT232HState;

namespace FT232H
{
	static const u32 MaxSendBytes = 0xFFFF + 1;
}

enum struct Signal : u8
{
	Low  = 0b0,
	High = 0b1,
};

b8     FT232H_Initialize          (FT232HState&);
void   FT232H_Teardown            (FT232HState&);

void   FT232H_SetTracing          (FT232HState&, b8 enable);
void   FT232H_SetDebugChecks      (FT232HState&, b8 enable);
b8     FT232H_HasError            (FT232HState&);

void   FT232H_Write               (FT232HState&, ByteSlice bytes);
void   FT232H_WriteImmediate      (FT232HState&, ByteSlice bytes);
void   FT232H_Flush               (FT232HState&);
void   FT232H_Read                (FT232HState&, Bytes& bytes, u16 numBytesToRead);
void   FT232H_SendBytes           (FT232HState&, ByteSlice bytes);
void   FT232H_RecvBytes           (FT232HState&, Bytes& bytes, u16 numBytesToRead);

u32    FT232H_SetClockSpeed       (FT232HState&, u32 hz);
u32    FT232H_GetClockSpeed       (FT232HState&);
void   FT232H_SetClockOverride    (FT232HState&, b8 enable, u32 hz);
void   FT232H_SetCLK              (FT232HState&, Signal signal);
void   FT232H_SetDO               (FT232HState&, Signal signal);
void   FT232H_SetCS               (FT232HState&, Signal signal);
void   FT232H_SetDC               (FT232HState&, Signal signal);
Signal FT232H_GetCLK              (FT232HState&);
Signal FT232H_GetDO               (FT232HState&);
Signal FT232H_GetCS               (FT232HState&);
Signal FT232H_GetDC               (FT232HState&);
void   FT232H_BeginSPI            (FT232HState&);
void   FT232H_EndSPI              (FT232HState&);
void   FT232H_BeginSPIDeferred    (FT232HState&);
void   FT232H_EndSPIDeferred      (FT232HState&);
