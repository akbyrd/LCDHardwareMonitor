#pragma once

#pragma unmanaged
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

//TODO: These aren't used currently
typedef void (*DataSourceInitialize)(List<Sensor>& sensors);
LHM_API void _cdecl Initialize(List<Sensor>&);

typedef void (*DataSourceUpdate)(List<Sensor>& sensors);
LHM_API void _cdecl Update(List<Sensor>&);

typedef void (*DataSourceTeardown)(List<Sensor>& sensors);
LHM_API void _cdecl Teardown(List<Sensor>&);
