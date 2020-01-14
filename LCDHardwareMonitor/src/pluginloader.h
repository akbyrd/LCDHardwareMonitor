struct PluginLoaderState;
struct Plugin;
struct SensorPlugin;
struct WidgetPlugin;

b8   PluginLoader_Initialize         (PluginLoaderState&);
void PluginLoader_Teardown           (PluginLoaderState&);
b8   PluginLoader_LoadSensorPlugin   (PluginLoaderState&, Plugin&, SensorPlugin&);
b8   PluginLoader_UnloadSensorPlugin (PluginLoaderState&, Plugin&, SensorPlugin&);
b8   PluginLoader_LoadWidgetPlugin   (PluginLoaderState&, Plugin&, WidgetPlugin&);
b8   PluginLoader_UnloadWidgetPlugin (PluginLoaderState&, Plugin&, WidgetPlugin&);
