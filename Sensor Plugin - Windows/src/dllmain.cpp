#include "LHMAPI.h"

static b8
Initialize(PluginContext& context, SensorPluginAPI::Initialize api)
{
	Unused(context, api);
	return true;
}

static void
Update(PluginContext& context, SensorPluginAPI::Update api)
{
	Unused(context, api);
}

static void
Teardown(PluginContext& context, SensorPluginAPI::Teardown api)
{
	Unused(context, api);
}

EXPORT void
GetSensorPluginInfo(PluginDesc& desc, SensorPluginFunctions& functions)
{
	desc.name       = "Windows Sensors";
	desc.author     = "akbyrd";
	desc.version    = 1;
	desc.lhmVersion = LHMVersion;

	functions.Initialize = Initialize;
	functions.Update     = Update;
	functions.Teardown   = Teardown;
}
