struct PluginLoaderState;
struct PluginInfo;
struct SensorPlugin;
struct WidgetPlugin;

b32  PluginLoader_Initialize         (PluginLoaderState*);
void PluginLoader_Teardown           (PluginLoaderState*);
b32  PluginLoader_LoadSensorPlugin   (PluginLoaderState*, PluginInfo*, SensorPlugin*);
b32  PluginLoader_UnloadSensorPlugin (PluginLoaderState*, PluginInfo*, SensorPlugin*);
b32  PluginLoader_LoadWidgetPlugin   (PluginLoaderState*, PluginInfo*, WidgetPlugin*);
b32  PluginLoader_UnloadWidgetPlugin (PluginLoaderState*, PluginInfo*, WidgetPlugin*);
