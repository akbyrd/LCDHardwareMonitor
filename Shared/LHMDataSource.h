#pragma once

#include "shared.hpp"

//TODO: This could be detected with a 'shim' of some sort (accessor function?)
/* You may *ONLY* access application provided services during scope of the function to which the services were passed. Basically, don't save a reference and access services from a separate thread.
 */

struct Sensor
{
	c16* name;
	c16* identifier;
	c16* displayFormat;
	r32  value;
};

typedef void (*DataSourceInitialize)(List<Sensor>& sensors);
LHM_API void _cdecl Initialize(List<Sensor>&);

typedef void (*DataSourceUpdate)(List<Sensor>& sensors);
LHM_API void _cdecl Update(List<Sensor>&);

typedef void (*DataSourceTeardown)(List<Sensor>& sensors);
LHM_API void _cdecl Teardown(List<Sensor>&);
