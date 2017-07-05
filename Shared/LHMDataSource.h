#pragma once

#include "shared.hpp"

//TODO: This could be detected with a 'shim' of some sort (accessor function?)
/* You may *ONLY* access application provided services during scope of the function to which the services were passed. Basically, don't save a reference and access services from a separate thread.
 */
typedef void (*DataSourceInitialize)(List<c16*>&);
LHM_API void _cdecl Initialize(List<c16*>& dataSources);

typedef void (*DataSourceUpdate)();
LHM_API void _cdecl Update();

typedef void (*DataSourceTeardown)();
LHM_API void _cdecl Teardown();
