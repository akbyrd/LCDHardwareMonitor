#pragma unmanaged
#include "LHMAPICLR.h"

#pragma managed
public interface class ISensorPlugin           { void GetPluginInfo (PluginDesc& desc); };
public interface class ISensorPluginInitialize { b8   Initialize    (PluginContext& context, SensorPluginAPI::Initialize api); };
public interface class ISensorPluginUpdate     { void Update        (PluginContext& context, SensorPluginAPI::Update     api); };
public interface class ISensorPluginTeardown   { void Teardown      (PluginContext& context, SensorPluginAPI::Teardown   api); };

public interface class IWidgetPlugin           { void GetPluginInfo (PluginDesc& desc); };
public interface class IWidgetPluginInitialize { b8   Initialize    (PluginContext& context, WidgetPluginAPI::Initialize api); };
public interface class IWidgetPluginUpdate     { void Update        (PluginContext& context, WidgetPluginAPI::Update     api); };
public interface class IWidgetPluginTeardown   { void Teardown      (PluginContext& context, WidgetPluginAPI::Teardown   api); };

public interface class IWidgetInitialize       { b8   Initialize    (PluginContext& context, WidgetAPI::Initialize api); };
public interface class IWidgetUpdate           { void Update        (PluginContext& context, WidgetAPI::Update     api); };
public interface class IWidgetTeardown         { void Teardown      (PluginContext& context, WidgetAPI::Teardown   api); };
#undef Attributes
