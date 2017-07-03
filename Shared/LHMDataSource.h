#pragma once

#include "shared.hpp"

typedef void (*DataSourceInitialize)();
LHM_API void Initialize();

typedef void (*DataSourceUpdate)();
LHM_API void Update();

typedef void (*DataSourceTeardown)();
LHM_API void Teardown();
