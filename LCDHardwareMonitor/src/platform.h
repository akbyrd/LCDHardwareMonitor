struct PlatformState;

enum struct Severity
{
	Info,
	Warning,
	Error
};

b32     Platform_Initialize        (PlatformState*);
void    Platform_Teardown          (PlatformState*);

void    Platform_Print             (c8* message);
void    Platform_Log               (c8* message, Severity severity, c8* file, i32 line, c8* function);
Bytes   Platform_LoadFileBytes     (c8* fileName);
String  Platform_LoadFileString    (c8* fileName);
// TODO: Might want to make ticks always signed
u64     Platform_GetTicks          ();
r32     Platform_TicksToSeconds    (i64 ticks);
r32     Platform_GetElapsedSeconds (u64 startTicks);

#define LOCATION_ARGS __FILE__, __LINE__, __FUNCTION__
#define LOG(message, severity) Platform_Log(message, severity, LOCATION_ARGS)
#define LOG_IF(expression, message, severity, ...) IF(expression, LOG(message, severity); __VA_ARGS__)
