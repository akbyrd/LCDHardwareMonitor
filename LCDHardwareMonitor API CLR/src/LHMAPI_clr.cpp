#pragma unmanaged
#include "LHMAPICLR.h"

#pragma managed
#define Attributes System::Runtime::InteropServices::ComVisible(false)
[Attributes] public interface class ISensorPlugin           { void GetPluginInfo (PluginInfo& info); };
[Attributes] public interface class ISensorPluginInitialize { b8   Initialize    (PluginContext& context, SensorPluginAPI::Initialize api); };
[Attributes] public interface class ISensorPluginUpdate     { void Update        (PluginContext& context, SensorPluginAPI::Update     api); };
[Attributes] public interface class ISensorPluginTeardown   { void Teardown      (PluginContext& context, SensorPluginAPI::Teardown   api); };

[Attributes] public interface class IWidgetPlugin           { void GetPluginInfo (PluginInfo& info); };
[Attributes] public interface class IWidgetPluginInitialize { b8   Initialize    (PluginContext& context, WidgetPluginAPI::Initialize api); };
[Attributes] public interface class IWidgetPluginUpdate     { void Update        (PluginContext& context, WidgetPluginAPI::Update     api); };
[Attributes] public interface class IWidgetPluginTeardown   { void Teardown      (PluginContext& context, WidgetPluginAPI::Teardown   api); };

[Attributes] public interface class IWidgetInitialize       { b8   Initialize    (PluginContext& context, WidgetAPI::Initialize api); };
[Attributes] public interface class IWidgetUpdate           { void Update        (PluginContext& context, WidgetAPI::Update     api); };
[Attributes] public interface class IWidgetTeardown         { void Teardown      (PluginContext& context, WidgetAPI::Teardown   api); };
#undef Attributes
