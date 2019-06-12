#pragma unmanaged
#include "LHMAPICLR.h"

#pragma managed
#define Attributes System::Runtime::InteropServices::ComVisible(false)
[Attributes] public interface class ISensorPlugin             { void GetPluginInfo (PluginInfo& info); };
[Attributes] public interface class ISensorInitialize         { b32  Initialize    (PluginContext& context, SensorPluginAPI::Initialize api); };
[Attributes] public interface class ISensorUpdate             { void Update        (PluginContext& context, SensorPluginAPI::Update     api); };
[Attributes] public interface class ISensorTeardown           { void Teardown      (PluginContext& context, SensorPluginAPI::Teardown   api); };

[Attributes] public interface class IWidgetPlugin             { void GetPluginInfo (PluginInfo& info); };
[Attributes] public interface class IWidgetInitialize         { b32  Initialize    (PluginContext& context, WidgetPluginAPI::Initialize api); };
[Attributes] public interface class IWidgetUpdate             { void Update        (PluginContext& context, WidgetPluginAPI::Update     api); };
[Attributes] public interface class IWidgetTeardown           { void Teardown      (PluginContext& context, WidgetPluginAPI::Teardown   api); };

[Attributes] public interface class IWidgetInstanceInitialize { b32  Initialize    (PluginContext& context, WidgetInstanceAPI::Initialize api); };
[Attributes] public interface class IWidgetInstanceUpdate     { void Update        (PluginContext& context, WidgetInstanceAPI::Update     api); };
[Attributes] public interface class IWidgetInstanceTeardown   { void Teardown      (PluginContext& context, WidgetInstanceAPI::Teardown   api); };
#undef Attributes
