namespace LCDHardwareMonitor {

struct PluginLoaderState;
struct SensorPlugin;
struct WidgetPlugin;

b8   PluginLoader_Initialize         (PluginLoaderState&);
void PluginLoader_Teardown           (PluginLoaderState&);
b8   PluginLoader_LoadSensorPlugin   (PluginLoaderState&, SensorPlugin&);
b8   PluginLoader_UnloadSensorPlugin (PluginLoaderState&, SensorPlugin&);
b8   PluginLoader_LoadWidgetPlugin   (PluginLoaderState&, WidgetPlugin&);
b8   PluginLoader_UnloadWidgetPlugin (PluginLoaderState&, WidgetPlugin&);

}
