#pragma once

//TODO: This could be detected with a 'shim' of some sort (accessor function?)
/* You may *ONLY* access application provided services during scope of the
 * function to which the services were passed. Basically, don't save a reference
 * and access services from a separate thread.
 */

struct Sensor
{
	c16* name;
	c16* identifier;
	c16* string;
	r32  value;
	//TODO: Range struct?
	r32  minValue;
	r32  maxValue;
};

typedef void (*DataSourceInitialize)(List<Sensor>& sensors);
LHM_API void _cdecl Initialize(List<Sensor>&);

typedef void (*DataSourceUpdate)(List<Sensor>& sensors);
LHM_API void _cdecl Update(List<Sensor>&);

typedef void (*DataSourceTeardown)(List<Sensor>& sensors);
LHM_API void _cdecl Teardown(List<Sensor>&);
