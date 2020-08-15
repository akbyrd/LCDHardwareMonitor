struct FT232HState;

b8   FT232H_Initialize     (FT232HState&);
void FT232H_Teardown       (FT232HState&);

void FT232H_SetTracing     (FT232HState&, b8 enable);
void FT232H_SetDebugChecks (FT232HState&, b8 enable);

void FT232H_Write          (FT232HState&, u8 command);
void FT232H_Write          (FT232HState&, ByteSlice bytes);
void FT232H_Read           (FT232HState&, Bytes& buffer, u32 numBytesToRead);
