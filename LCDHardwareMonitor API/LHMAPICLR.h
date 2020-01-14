#ifndef LHM_APICLR
#define LHM_APICLR

#pragma unmanaged
struct PluginContext {};

#include "LHMAPI.h"

#pragma managed
#pragma make_public(PluginContext)
#pragma make_public(PluginInfo)
#pragma make_public(SensorPluginAPI)
#pragma make_public(WidgetPluginAPI)
#pragma make_public(WidgetAPI)

using CLRString = System::String;
using LHMString = ::String;

b8
String_FromCLR(LHMString lhmString, CLRString^ clrString)
{
	using namespace System;
	using namespace System::Runtime::InteropServices;

	c8* cstring = (c8*) Marshal::StringToHGlobalAnsi(clrString).ToPointer();

	u32 length = (u32) clrString->Length + 1;
	b8 success = String_Reserve(lhmString, length);
	if (!success) return false;

	lhmString.length = length;
	strncpy_s(&lhmString[0], lhmString.capacity, cstring, length);

	Marshal::FreeHGlobal((IntPtr) cstring);
	return true;
}

#endif
