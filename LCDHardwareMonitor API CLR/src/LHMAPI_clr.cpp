#pragma unmanaged
#include "LHMAPICLR.h"

#pragma managed
#define Attributes System::Runtime::InteropServices::ComVisible(false)
[Attributes] public interface class ISensorPlugin {};
[Attributes] public interface class ISensorInitialize { void Initialize (PluginContext* context, SensorPlugin::InitializeAPI* api); };
[Attributes] public interface class ISensorUpdate     { void Update     (PluginContext* context, SensorPlugin::UpdateAPI*     api); };
[Attributes] public interface class ISensorTeardown   { void Teardown   (PluginContext* context, SensorPlugin::TeardownAPI*   api); };

[Attributes] public interface class IWidgetPlugin {};
[Attributes] public interface class IWidgetInitialize { void Initialize (PluginContext* context, WidgetPlugin::InitializeAPI* api); };
[Attributes] public interface class IWidgetUpdate     { void Update     (PluginContext* context, WidgetPlugin::UpdateAPI*     api); };
[Attributes] public interface class IWidgetTeardown   { void Teardown   (PluginContext* context, WidgetPlugin::TeardownAPI*   api); };
#undef Attributes
