struct PluginLoaderState;
struct PluginHeader;
struct SensorPlugin;
struct WidgetPlugin;

b32  PluginLoader_Initialize         (PluginLoaderState*);
void PluginLoader_Teardown           (PluginLoaderState*);
b32  PluginLoader_LoadSensorPlugin   (PluginLoaderState*, PluginHeader*, SensorPlugin*);
b32  PluginLoader_UnloadSensorPlugin (PluginLoaderState*, PluginHeader*, SensorPlugin*);
b32  PluginLoader_LoadWidgetPlugin   (PluginLoaderState*, PluginHeader*, WidgetPlugin*);
b32  PluginLoader_UnloadWidgetPlugin (PluginLoaderState*, PluginHeader*, WidgetPlugin*);
