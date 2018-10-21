#pragma unmanaged
#include "LHMAPI.h"
#include "LHMPluginHeader.h"
#include "LHMSensorPlugin.h"
#include "LHMWidgetPlugin.h"

// TODO: Restructure these to allow plugins to avoid implementing functions
// they don't need. Don't spend time calling empty functions.

#pragma managed
public interface class ISensorPlugin
{
	void Initialize (SP_INITIALIZE_ARGS);
	void Update     (SP_UPDATE_ARGS);
	void Teardown   (SP_TEARDOWN_ARGS);
};

public interface class IWidgetPlugin
{
	void Initialize (WP_INITIALIZE_ARGS);
	void Update     (WP_UPDATE_ARGS);
	void Teardown   (WP_TEARDOWN_ARGS);
};
