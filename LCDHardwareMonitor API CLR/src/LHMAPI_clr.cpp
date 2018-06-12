#pragma unmanaged
#include "LHMAPI.h"

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
