struct PlatformState;

b32     InitializePlatform (PlatformState*);
void    TeardownPlatform   (PlatformState*);

b32     LoadFile           (c16* fileName, unique_ptr<c8[]>& data, size& dataSize);
void    ConsolePrint       (c16* message);

struct Plugin;
Plugin* LoadPlugin         (PlatformState*, c16* directory, c16* name);
b32     UnloadPlugin       (PlatformState*, Plugin* plugin);
void*   GetPluginSymbol    (Plugin* plugin, c8* symbol);