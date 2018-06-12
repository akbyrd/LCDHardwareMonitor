#pragma unmanaged
#include "LHMAPI.h"

#pragma managed
public interface class IDataSourcePlugin
{
	void Initialize (DS_INITIALIZE_ARGS);
	void Update     (DS_UPDATE_ARGS);
	void Teardown   (DS_TEARDOWN_ARGS);
};
