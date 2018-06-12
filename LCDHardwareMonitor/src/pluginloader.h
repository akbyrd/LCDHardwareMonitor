struct PluginLoaderState;

b32  PluginLoader_Initialize         (PluginLoaderState*);
void PluginLoader_Teardown           (PluginLoaderState*);
b32  PluginLoader_LoadSensorPlugin   (PluginLoaderState*, PluginHeader*, SensorPlugin*);
b32  PluginLoader_UnloadSensorPlugin (PluginLoaderState*, PluginHeader*, SensorPlugin*);
