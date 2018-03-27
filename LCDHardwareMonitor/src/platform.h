struct PlatformState;

b32     Platform_Initialize (PlatformState*);
void    Platform_Teardown   (PlatformState*);

Bytes   LoadFileBytes       (c16* fileName);
String  LoadFileString      (c16* fileName);
WString LoadFileWString     (c16* fileName);
void    ConsolePrint        (c16* message);

struct Plugin;
Plugin* LoadPlugin          (PlatformState*, c16* directory, c16* name);
b32     UnloadPlugin        (PlatformState*, Plugin* plugin);
void*   GetPluginSymbol     (Plugin* plugin, c8* symbol);
