#ifndef LHM_APICLR
#define LHM_APICLR

#pragma unmanaged
struct PluginContext {};

#include "LHMAPI.h"

#pragma managed
#pragma make_public(PluginContext)
#pragma make_public(PluginDesc)
#pragma make_public(SensorPluginAPI)
#pragma make_public(WidgetPluginAPI)
#pragma make_public(WidgetAPI)

using CLRString = System::String;
using LHMString = ::String;

template<typename ...Args>
constexpr inline void Unused(Args^ ...) {}

void
String_FromCLR(LHMString& lhmString, CLRString^ clrString)
{
	using namespace System;
	using namespace System::Runtime::InteropServices;

	c8* cstring = (c8*) Marshal::StringToHGlobalAnsi(clrString).ToPointer();

	u32 length = (u32) clrString->Length;
	String_Reserve(lhmString, length + 1);

	lhmString.length = length;
	strncpy_s(lhmString.data, lhmString.capacity, cstring, length + 1);

	Marshal::FreeHGlobal((IntPtr) cstring);
}

StringView
StringView_PinCLR(CLRString^ clrString)
{
	using namespace System::Runtime::InteropServices;

	StringView string = {};
	string.length = (u32) clrString->Length;
	string.data = (c8*) Marshal::StringToHGlobalAnsi(clrString).ToPointer();
	return string;
}

void
StringView_ReleaseCLR(StringView& string)
{
	using namespace System;
	using namespace System::Runtime::InteropServices;

	Marshal::FreeHGlobal((IntPtr) string.data);
	string = {};
}

#endif
