#pragma unmanaged
#include "LHMAPI.h"

#pragma managed
public interface class ISensorPlugin
{
	void Initialize (SP_INITIALIZE_ARGS);
	void Update     (SP_UPDATE_ARGS);
	void Teardown   (SP_TEARDOWN_ARGS);
};
