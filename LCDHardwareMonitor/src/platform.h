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
	c8* file;
	i32 line;
	c8* function;
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

template<typename... Args>
void       Platform_Print                  (c8* format, Args... args);
void       Platform_Print                  (c8* message);

template<typename... Args>
void       Platform_Log                    (Severity severity, Location location, c8* format, Args... args);
void       Platform_Log                    (Severity severity, Location location, c8* message);

Bytes      Platform_LoadFileBytes          (c8* path);
String     Platform_LoadFileString         (c8* path);
i64        Platform_GetTicks               ();
r32        Platform_TicksToSeconds         (i64 ticks);
r32        Platform_GetElapsedSeconds      (i64 startTicks);
r32        Platform_GetElapsedMilliseconds (i64 startTicks);
void       Platform_RequestQuit            ();

PipeResult Platform_CreatePipeServer       (StringSlice name, Pipe*);
PipeResult Platform_CreatePipeClient       (StringSlice name, Pipe*);
void       Platform_DestroyPipe            (Pipe*);
PipeResult Platform_ConnectPipe            (Pipe*);
PipeResult Platform_DisconnectPipe         (Pipe*);
PipeResult Platform_UpdatePipeConnection   (Pipe*);
PipeResult Platform_WritePipe              (Pipe*, Bytes bytes);
PipeResult Platform_ReadPipe               (Pipe*, Bytes& bytes);
PipeResult Platform_FlushPipe              (Pipe*);

#define LOCATION { __FILE__, __LINE__, __FUNCTION__ }
#define LOG(severity, format, ...) Platform_Log(severity, LOCATION, format, __VA_ARGS__)
#define LOG_IF(expression, action, severity, format, ...) IF(expression, LOG(severity, format, __VA_ARGS__); action)
