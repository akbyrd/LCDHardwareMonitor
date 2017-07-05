#pragma managed
using namespace System::Diagnostics;
using namespace System::Runtime::InteropServices;
using namespace OpenHardwareMonitor;
using namespace OpenHardwareMonitor::Hardware;

ref struct State
{
	static Computer computer;
};

#define EXPORTING 1
#include "LHMDataSource.h"

void
Initialize(List<c16*>& dataSources)
{
	Debug::Write(L"Plugin Initialize!\n");
	Computer^ computer = %State::computer;

	//ENABLE ALL THE THINGS!
	computer->CPUEnabled           = true;
	computer->FanControllerEnabled = true;
	computer->GPUEnabled           = true;
	computer->HDDEnabled           = true;
	computer->MainboardEnabled     = true;
	computer->RAMEnabled           = true;

	computer->Open();

	array<IHardware^>^ hardware = computer->Hardware;
	for (i32 i = 0; i < hardware->Length; i++)
	{
		//TODO: Need to free these strings
		c16* name = (c16*) Marshal::StringToHGlobalUni(hardware[i]->Name).ToPointer();
		List_Append(dataSources, name);
	}

	//computer->HardwareAdded   += OnHardwareAdded;
	//computer->HardwareRemoved += OnHardwareRemoved;
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