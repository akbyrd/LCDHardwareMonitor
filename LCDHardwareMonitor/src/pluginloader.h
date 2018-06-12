struct PluginLoaderState;

b32  PluginLoader_Initialize       (PluginLoaderState*);
void PluginLoader_Teardown         (PluginLoaderState*);
b32  PluginLoader_LoadDataSource   (PluginLoaderState*, PluginHeader*, DataSource*);
b32  PluginLoader_UnloadDataSource (PluginLoaderState*, PluginHeader*, DataSource*);
