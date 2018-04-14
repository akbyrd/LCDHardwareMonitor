struct Plugin;
struct PluginLoaderState;

b32     PluginLoader_Initialize      (PluginLoaderState*);
void    PluginLoader_Teardown        (PluginLoaderState*);
Plugin* PluginLoader_LoadPlugin      (PluginLoaderState*, c16* directory, c16* name);
b32     PluginLoader_UnloadPlugin    (PluginLoaderState*, Plugin*);
void*   PluginLoader_GetPluginSymbol (PluginLoaderState*, Plugin*, c8* symbol);
