enum struct Severity
{
	Null,
	Info,
	Warning,
	Error,
	Fatal,
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
	b32       isServer;
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

Bytes      Platform_LoadFileBytes          (StringView path);
String     Platform_LoadFileString         (StringView path);
i64        Platform_GetTicks               ();
r32        Platform_TicksToSeconds         (i64 ticks);
r32        Platform_GetElapsedSeconds      (i64 startTicks);
r32        Platform_GetElapsedMilliseconds (i64 startTicks);
void       Platform_RequestQuit            ();

PipeResult Platform_CreatePipeServer       (StringView name, Pipe&);
PipeResult Platform_CreatePipeClient       (StringView name, Pipe&);
void       Platform_DestroyPipe            (Pipe&);
PipeResult Platform_ConnectPipe            (Pipe&);
PipeResult Platform_DisconnectPipe         (Pipe&);
PipeResult Platform_UpdatePipeConnection   (Pipe&);
PipeResult Platform_WritePipe              (Pipe&, Bytes bytes);
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
