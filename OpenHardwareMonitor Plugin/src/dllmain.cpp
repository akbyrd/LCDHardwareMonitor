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

	void OHM_Initialize()
	{
		Debug::WriteLine(L"Managed Initialize!\n");

		Managed_Memory state;
	}
}

#pragma unmanaged
namespace Native
{
	#include <wtypes.h>
	//#include "LHMDataSource.h"

	void Initialize()
	{
		OutputDebugStringW(L"Native Initialize!\n");

		Managed::OHM_Initialize();
	}
}