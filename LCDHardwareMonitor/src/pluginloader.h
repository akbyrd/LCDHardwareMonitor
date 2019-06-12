struct PluginLoaderState;
struct SensorPlugin;
struct WidgetPlugin;

b32  PluginLoader_Initialize         (PluginLoaderState&);
void PluginLoader_Teardown           (PluginLoaderState&);
b32  PluginLoader_LoadSensorPlugin   (PluginLoaderState&, SensorPlugin&);
b32  PluginLoader_UnloadSensorPlugin (PluginLoaderState&, SensorPlugin&);
b32  PluginLoader_LoadWidgetPlugin   (PluginLoaderState&, WidgetPlugin&);
b32  PluginLoader_UnloadWidgetPlugin (PluginLoaderState&, WidgetPlugin&);
