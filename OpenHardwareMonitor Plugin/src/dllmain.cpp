#pragma managed
using namespace System::Diagnostics;
using namespace OpenHardwareMonitor;
using namespace OpenHardwareMonitor::Hardware;

ref struct State
{
	static Computer computer;
};

#define EXPORTING 1
#include "LHMDataSource.h"

void
Initialize()
{
	Debug::Write(L"Plugin Initialize!\n");

	State::computer.Open();
}

void
Update()
{
	Debug::Write(L"Plugin Update!\n");
}

void
Teardown()
{
	Debug::Write(L"Plugin Teardown!\n");
}