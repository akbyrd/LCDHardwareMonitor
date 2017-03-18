#pragma managed
using namespace OpenHardwareMonitor;
using namespace OpenHardwareMonitor::Hardware;

ref struct Managed_Memory
{
	Computer computer;
};

//Managed_Memory state;

void
Managed_Initialize ()
{
	Managed_Memory state;

	//ENABLE ALL THE THINGS!
	state.computer.MainboardEnabled     = true;
	state.computer.CPUEnabled           = true;
	state.computer.RAMEnabled           = true;
	state.computer.GPUEnabled           = true;
	state.computer.FanControllerEnabled = true;
	state.computer.HDDEnabled           = true;

	//state.computer.Open();
}

#if false
public void VisitComputer ( IComputer computer )
{
	computer.Traverse(this);
}

public void VisitHardware ( IHardware hardware )
{
	hardware.Update();
	foreach ( IHardware subHardware in hardware.SubHardware )
		subHardware.Accept(this);
}

public void VisitSensor ( ISensor sensor ) { }

public void VisitParameter ( IParameter parameter ) { }
#endif

#pragma unmanaged
#include <Windows.h>

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	OutputDebugStringW(L"LOADED!");
	return true;
}

extern "C"
{
	_declspec(dllexport)
	void
	Initialize()
	{
		OutputDebugStringW(L"INITIALIZE!");
		//Managed_Initialize();
	}
}