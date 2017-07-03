#pragma managed
namespace Managed
{
	using namespace System::Diagnostics;
	using namespace OpenHardwareMonitor;
	using namespace OpenHardwareMonitor::Hardware;
	using namespace System;
	using namespace System::Reflection;

	ref struct Managed_Memory
	{
		Computer computer;
	};
	//Managed_Memory state;

	Assembly^ AssemblyResolveHandler(Object^ sender, ResolveEventArgs^ args)
	{
		if (args->Name->Contains("OpenHardwareMonitorLib"))
			return Assembly::LoadFrom("Data Sources\\OpenHardwareMonitor Source\\OpenHardwareMonitorLib.dll");
		return nullptr;
	}

	void OHM_Initialize()
	{
		Debug::WriteLine(L"Managed Initialize!\n");
		AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(AssemblyResolveHandler);

		Managed_Memory state;
	}
}

#pragma unmanaged
//@TODO: Research how exporting works with namespaces
#include <wtypes.h>
//#define EXPORTING 1
//#include "LHMDataSource.h"

namespace Native
{
	extern "C" __declspec(dllexport)
	void _cdecl
	Initialize()
	{
		OutputDebugStringW(L"Native Initialize!\n");

		Managed::OHM_Initialize();
	}

	extern "C" __declspec(dllexport)
	void _cdecl
	Teardown()
	{
		OutputDebugStringW(L"Native Teardown!\n");
	}
}