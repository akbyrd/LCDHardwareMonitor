#pragma unmanaged
#include "LHMAPI.h"
#include "LHMPluginHeader.h"
#include "LHMSensorPlugin.h"
#include "LHMWidgetPlugin.h"

#pragma managed
#define Attributes System::Runtime::InteropServices::ComVisible(false)
[Attributes] public interface class ISensorPlugin {};
[Attributes] public interface class ISensorInitialize { void Initialize (SP_INITIALIZE_ARGS); };
[Attributes] public interface class ISensorUpdate     { void Update     (SP_UPDATE_ARGS);     };
[Attributes] public interface class ISensorTeardown   { void Teardown   (SP_TEARDOWN_ARGS);   };

[Attributes] public interface class IWidgetPlugin {};
[Attributes] public interface class IWidgetInitialize { void Initialize (WP_INITIALIZE_ARGS); };
[Attributes] public interface class IWidgetUpdate     { void Update     (WP_UPDATE_ARGS);     };
[Attributes] public interface class IWidgetTeardown   { void Teardown   (WP_TEARDOWN_ARGS);   };
#undef Attributes
