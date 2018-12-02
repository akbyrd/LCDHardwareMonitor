struct PlatformState;

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

b32     Platform_Initialize        (PlatformState*);
void    Platform_Teardown          (PlatformState*);

template<typename... Args>
void    Platform_Print             (c8* format, Args... args);
void    Platform_Print             (c8* message);

template<typename... Args>
void    Platform_Log               (Severity severity, Location location, c8* format, Args... args);
void    Platform_Log               (Severity severity, Location location, c8* message);

Bytes   Platform_LoadFileBytes     (c8* path);
String  Platform_LoadFileString    (c8* path);
// TODO: Might want to make ticks always signed
u64     Platform_GetTicks          ();
r32     Platform_TicksToSeconds    (i64 ticks);
r32     Platform_GetElapsedSeconds (u64 startTicks);

#define LOCATION { __FILE__, __LINE__, __FUNCTION__ }
#define LOG(severity, format, ...) Platform_Log(severity, LOCATION, format, __VA_ARGS__)
#define LOG_IF(expression, action, severity, format, ...) IF(expression, LOG(severity, format, __VA_ARGS__); action)
