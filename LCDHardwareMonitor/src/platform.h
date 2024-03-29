enum struct Severity
{
	Null,
	Info,
	Warning,
	Error,
	Fatal,
};

enum struct DLLState
{
	Null,
	NotFound,
	Found,
};

struct Location
{
	StringView file;
	i32        line;
	StringView function;
};

enum struct PipeState
{
	Null,
	Connecting,
	Connected,
	Disconnecting,
	Disconnected,
};

struct PipeImpl;
struct Pipe
{
	String    name;
	PipeState state;
	b8        isServer;
	PipeImpl* impl;
};

enum struct PipeResult
{
	Null,
	Success,
	TransientFailure,
	UnexpectedFailure,
};

#define Platform_Print(format, ...) \
	Platform_PrintChecked<CountPlaceholders(format)>(format, ##__VA_ARGS__)

template<u32 PlaceholderCount, typename... Args>
inline void
Platform_PrintChecked(StringView format, Args... args);

#define Platform_Log(severity, location, format, ...) \
	Platform_LogChecked<CountPlaceholders(format)>(severity, location, format, ##__VA_ARGS__)

template<u32 PlaceholderCount, typename... Args>
inline void
Platform_LogChecked(Severity severity, Location location, StringView format, Args... args);

b8         Platform_WriteFileBytes         (StringView path, ByteSlice bytes);
Bytes      Platform_LoadFileBytes          (StringView path);
String     Platform_LoadFileString         (StringView path);
i64        Platform_GetTicks               ();
r32        Platform_TicksToSeconds         (i64 ticks);
i64        Platform_SecondsToTicks         (r32 seconds);
r32        Platform_GetElapsedSeconds      (i64 startTicks);
r32        Platform_GetElapsedSeconds      (i64 startTicks, i64 endTicks);
r32        Platform_GetElapsedMilliseconds (i64 startTicks);
r32        Platform_GetElapsedMilliseconds (i64 startTicks, i64 endTicks);
void       Platform_Sleep                  (u32 ms);
void       Platform_RequestQuit            ();

PipeResult Platform_CreatePipeServer       (StringView name, Pipe&);
PipeResult Platform_CreatePipeClient       (StringView name, Pipe&);
void       Platform_DestroyPipe            (Pipe&);
PipeResult Platform_ConnectPipe            (Pipe&);
PipeResult Platform_DisconnectPipe         (Pipe&);
PipeResult Platform_UpdatePipeConnection   (Pipe&);
PipeResult Platform_WritePipe              (Pipe&, ByteSlice bytes);
PipeResult Platform_ReadPipe               (Pipe&, Bytes& bytes);
PipeResult Platform_FlushPipe              (Pipe&);

#define LOCATION { __FILE__, __LINE__, __FUNCTION__ }
#if true
#define LOG(severity, format, ...) Platform_Log(severity, LOCATION, format, __VA_ARGS__)
#define LOG_IF(expression, action, severity, format, ...) IF(expression, LOG(severity, format, __VA_ARGS__); action)
#else
#define LOG(severity, format, ...)
#define LOG_IF(expression, action, severity, format, ...)
#endif

// TODO: This belongs in LHMBytes, but it introduces a dependency on platform.h which currently
// isn't part of the public API.
void
Bytes_Print(StringView prefix, ByteSlice bytes)
{
	if (bytes.length == 0) return;

	String string = {};
	defer { String_Free(string); };

	for (u32 i = 0; i < 2; i++)
	{
		u32 length = 0;
		length += ToStringf(string, "%s ", prefix.data);
		length += ToStringf(string, "0x%.2X", bytes[0]);
		for (u32 j = 1; j < bytes.length; j++)
			length += ToStringf(string, " 0x%.2X", bytes[j]);

		if (i == 0)
			String_Reserve(string, length + 2);
	}
	string.data[string.length++] = '\n';
	string.data[string.length] = '\0';

	// TODO: Doesn't work :(
	//Platform_Print(string);

	Platform_Print("%", string.data);
}
