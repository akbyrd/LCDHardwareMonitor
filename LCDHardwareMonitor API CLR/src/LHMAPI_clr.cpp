#pragma unmanaged
#include "LHMAPI.h"
#include "LHMPluginHeader.h"
#include "LHMSensorPlugin.h"
#include "LHMWidgetPlugin.h"

#pragma managed
public interface class ISensorPlugin { };
public interface class ISensorInitialize { void Initialize (SP_INITIALIZE_ARGS); };
public interface class ISensorUpdate     { void Update     (SP_UPDATE_ARGS);     };
public interface class ISensorTeardown   { void Teardown   (SP_TEARDOWN_ARGS);   };

public interface class IWidgetPlugin { };
public interface class IWidgetInitialize { void Initialize (WP_INITIALIZE_ARGS); };
public interface class IWidgetUpdate     { void Update     (WP_UPDATE_ARGS);     };
public interface class IWidgetTeardown   { void Teardown   (WP_TEARDOWN_ARGS);   };
