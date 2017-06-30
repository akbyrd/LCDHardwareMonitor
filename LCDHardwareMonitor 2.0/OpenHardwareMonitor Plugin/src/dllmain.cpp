#pragma managed
namespace Managed
{
	using namespace System::Diagnostics;
	using namespace OpenHardwareMonitor;
	using namespace OpenHardwareMonitor::Hardware;

	ref struct Managed_Memory
	{
		Computer computer;
	};

	//Managed_Memory state;

	void OHM_Initialize();

	void
	Managed_Initialize ()
	{
		Debug::WriteLine(L"Managed Initialize!\n");

		//OHM_Initialize();
	}

	void OHM_Initialize()
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
}

#pragma unmanaged
namespace Native
{
	#include <wtypes.h>

	BOOL WINAPI
	DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
	{
		switch (fdwReason)
		{ 
			case DLL_PROCESS_ATTACH:
				break;

			case DLL_THREAD_ATTACH:
				break;

			case DLL_THREAD_DETACH:
				break;

			case DLL_PROCESS_DETACH:
				break;
		}

		return true;
	}


	extern "C" _declspec(dllexport)
	void
	Initialize()
	{
		OutputDebugStringW(L"Native Initialize!\n");

		Managed::Managed_Initialize();
	}
}