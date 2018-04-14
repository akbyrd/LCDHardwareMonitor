struct PlatformState;

enum struct Severity
{
	Info,
	Warning,
	Error
};

b32     Platform_Initialize      (PlatformState*);
void    Platform_Teardown        (PlatformState*);

Bytes   Platform_LoadFileBytes   (c16* fileName);
String  Platform_LoadFileString  (c16* fileName);
WString Platform_LoadFileWString (c16* fileName);
void    Platform_Print           (c16* message);
void    Platform_Log             (c16* message, Severity severity, c16* file, i32 line, c16* function);

#define LOG(message, severity) Platform_Log(message, severity, WFILE, LINE, WFUNC)
#define LOG_IF(expression, message, severity, ...) IF(expression, LOG(message, severity); __VA_ARGS__)
