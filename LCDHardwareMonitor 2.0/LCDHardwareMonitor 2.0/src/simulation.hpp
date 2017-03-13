enum struct Severity
{
	Info,
	Warning,
	Error
};


#define LOG_IF(expression, string, reaction) \
if (expression)                              \
{                                            \
	LOG(string, Severity::Error);            \
	reaction;                                \
}                                            \

#define NOTHING

#define WIDEHELPER(x) L##x
#define WIDE(x) WIDEHELPER(x)

#define LOG(message, severity) Log(message, severity, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__))
void
Log(c16* message, Severity severity, c16* file, i32 line, c16* function)
{
	ConsolePrint(message);
	ConsolePrint(L"\n\t");
	ConsolePrint(file);
	//ConsolePrint(line);
	ConsolePrint(function);

	__debugbreak();
}

struct SimulationState
{
	V2i renderSize = {320, 240};
};