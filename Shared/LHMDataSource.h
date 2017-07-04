#pragma once

#include "shared.hpp"

typedef void (*DataSourceInitialize)();
LHM_API void _cdecl Initialize();

typedef void (*DataSourceUpdate)();
LHM_API void _cdecl Update();

typedef void (*DataSourceTeardown)();
LHM_API void _cdecl Teardown();
