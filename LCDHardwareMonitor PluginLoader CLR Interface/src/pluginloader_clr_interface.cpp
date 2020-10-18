#pragma unmanaged
using b8 = bool;

struct Plugin {};
struct SensorPlugin {};
struct WidgetPlugin {};

#pragma managed
#pragma make_public(Plugin)
#pragma make_public(SensorPlugin)
#pragma make_public(WidgetPlugin)

using namespace System::Runtime::InteropServices;

// TODO: It'd be simpler to define this in an IDL file, but I cannot for the life of me figure out
// how to pass native types without defining them in IDL.
[ComVisible(true), InterfaceType(ComInterfaceType::InterfaceIsIUnknown)]
public interface class
ILHMPluginLoader
{
	b8 LoadSensorPlugin   (Plugin& plugin, SensorPlugin& sensorPlugin);
	b8 UnloadSensorPlugin (Plugin& plugin, SensorPlugin& sensorPlugin);
	b8 LoadWidgetPlugin   (Plugin& plugin, WidgetPlugin& widgetPlugin);
	b8 UnloadWidgetPlugin (Plugin& plugin, WidgetPlugin& widgetPlugin);
};
