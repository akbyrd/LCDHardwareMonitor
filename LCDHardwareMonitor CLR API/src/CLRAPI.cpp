#pragma unmanaged
#include "LHMAPI.h"

#pragma managed
public interface class IDataSourcePlugin
{
	void Initialize ();
	void Update     ();
	void Teardown   ();
};
