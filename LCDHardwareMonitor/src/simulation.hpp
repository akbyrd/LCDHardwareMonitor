enum struct Severity
{
	Info,
	Warning,
	Error
};

#define WIDEHELPER(x) L##x
#define WIDE(x) WIDEHELPER(x)

#define LOG(message, severity) Log(message, severity, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__))
#define LOG_IF(expression, message, severity, ...) IF(expression, LOG(message, severity); __VA_ARGS__)
//#define LOGI_IF(expression, message, ...) LOG_IF(expression, message, Severity::Info,    ##__VA_ARGS__)
//#define LOGW_IF(expression, message, ...) LOG_IF(expression, message, Severity::Warning, ##__VA_ARGS__)
//#define LOGE_IF(expression, message, ...) LOG_IF(expression, message, Severity::Error,   ##__VA_ARGS__)
void
Log(c16* message, Severity severity, c16* file, i32 line, c16* function)
{
	//TODO: Decide on logging allocation policy
	//TODO: Check for swprintf failure (overflow).
	c16 buffer[512];
	swprintf(buffer, ArrayCount(buffer), L"%s - %s\n\t%s(%i)\n", function, message, file, line);
	ConsolePrint(buffer);

	if (severity > Severity::Info)
		__debugbreak();
}

struct Widget
{
	Sensor* sensor;
	Mesh mesh;
	V2i size;
	float minValue;
	float maxValue;
	//c16* displayFormat;
};

struct SimulationState
{
	V2i renderSize = {320, 240};
	Plugin plugin;
	Widget widget;
};

void
Simulation_Initialize(SimulationState* s)
{

}

//TODO: Do something about D3DRenderState being known here.
void
Simulation_Update(SimulationState* s, D3DRendererState* rendererState)
{
	s->plugin.update(s->plugin.sensors);

	Widget& w = s->widget;
	w.sensor   = s->plugin.sensors.items + 0;
	w.mesh     = Mesh::Quad;
	w.minValue = 0;
	w.maxValue = 100;

	DrawCall* dc = PushDrawCall(rendererState);
	dc->mesh = w.mesh;
	float sizeX = w.size.x * (w.sensor->value - w.minValue) / (w.maxValue - w.minValue);
	XMStoreFloat4x4((XMFLOAT4X4*) &dc->worldM, XMMatrixScaling(sizeX, w.size.y, 1) * XMMatrixIdentity());
}