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
	Debug::WriteLine(L"Plugin Initialize!\n");

	State::computer.Open();
}

void
Update()
{
	Debug::WriteLine(L"Plugin Update!\n");
}

void
Teardown()
{
	Debug::WriteLine(L"Plugin Teardown!\n");
}