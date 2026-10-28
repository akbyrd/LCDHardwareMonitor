#include "LHMAPI.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <stdio.h>
#pragma comment(lib, "pdh.lib")

static void Teardown(PluginContext& context, SensorPluginAPI::Teardown api);

enum struct InstanceType
{
	Load,
	Clock,
	Power,
};

struct Instance
{
	InstanceType type;
	String       name;
};

struct State
{
	static const u64 sampleIntervalMs = 1000;

	PDH_HQUERY     query;
	PDH_HCOUNTER   utility;
	PDH_HCOUNTER   frequency;
	PDH_HCOUNTER   power;
	List<Instance> instances;
	Bytes          buffer;
	u64            lastSampleMs;
};

static State state = {};

static Slice<PDH_RAW_COUNTER_ITEM_A>
GetInstances(PDH_HCOUNTER counter)
{
	DWORD size  = 0;
	DWORD count = 0;
	PDH_STATUS status = PdhGetRawCounterArrayA(counter, &size, &count, nullptr);
	if (status != PDH_MORE_DATA) return {};

	List_Reserve(state.buffer, size);
	Slice items((PDH_RAW_COUNTER_ITEM_A*) state.buffer.data, count);
	status = PdhGetRawCounterArrayA(counter, &size, &count, items.data);
	if (status != ERROR_SUCCESS) return {};

	return items;
}

static Slice<PDH_FMT_COUNTERVALUE_ITEM_A>
GetValues(PDH_HCOUNTER counter)
{
	DWORD size  = 0;
	DWORD count = 0;
	DWORD format = PDH_FMT_DOUBLE | PDH_FMT_NOCAP100;
	PDH_STATUS status = PdhGetFormattedCounterArrayA(counter, format, &size, &count, nullptr);
	if (status != PDH_MORE_DATA) return {};

	List_Reserve(state.buffer, size);
	Slice items((PDH_FMT_COUNTERVALUE_ITEM_A*) state.buffer.data, count);
	status = PdhGetFormattedCounterArrayA(counter, format, &size, &count, items.data);
	if (status != ERROR_SUCCESS) return {};

	return items;
}

static b8
IsValid(PDH_FMT_COUNTERVALUE_ITEM_A& item)
{
	return item.FmtValue.CStatus == PDH_CSTATUS_VALID_DATA || item.FmtValue.CStatus == PDH_CSTATUS_NEW_DATA;
}

static Instance*
FindInstance(InstanceType type, const c8* name)
{
	for (u32 i = 0; i < state.instances.length; i++)
	{
		Instance& instance = state.instances[i];
		if (instance.type == type && strcmp(instance.name.data, name) == 0)
			return &instance;
	}
	return nullptr;
}

static void
AddSensor(
	PluginContext&               context,
	SensorPluginAPI::Initialize& api,
	InstanceType                 type,
	StringView                   instanceName,
	StringView                   name,
	StringView                   identifier,
	StringView                   format
)
{
	Instance& instance = List_Append(state.instances);
	instance.type = type;
	instance.name = String_FromView(instanceName);

	SensorDesc desc = {};
	desc.name       = name;
	desc.identifier = identifier;
	desc.format     = format;
	api.RegisterSensors(context, desc);
}

static b8
Initialize(PluginContext& context, SensorPluginAPI::Initialize api)
{
	PDH_STATUS status = PdhOpenQueryA(nullptr, 0, &state.query);
	if (status != ERROR_SUCCESS) return false;

	auto queryGuard = guard { Teardown(context, {}); };

	status = PdhAddEnglishCounterA(state.query, "\\Processor Information(*)\\% Processor Utility", 0, &state.utility);
	if (status != ERROR_SUCCESS) return false;
	status = PdhAddEnglishCounterA(state.query, "\\Processor Information(*)\\Actual Frequency", 0, &state.frequency);
	if (status != ERROR_SUCCESS) return false;
	status = PdhAddEnglishCounterA(state.query, "\\Energy Meter(*)\\Power", 0, &state.power);
	if (status != ERROR_SUCCESS) state.power = nullptr;

	status = PdhCollectQueryData(state.query);
	if (status != ERROR_SUCCESS) return false;
	state.lastSampleMs = GetTickCount64();

	c8 nameBuf[64];
	c8 identifierBuf[64];
	StringView name = {};
	StringView identifier = {};

	ULONG highestNode = 0;
	GetNumaHighestNodeNumber(&highestNode);

	// _Total     - All logical processors
	// <N>,_Total - All logical processors in node
	// <N>,<N>    - One logical processor in node
	Slice<PDH_RAW_COUNTER_ITEM_A> processors = GetInstances(state.utility);
	for (u32 i = 0; i < processors.length; i++)
	{
		StringView instanceName(processors[i].szName, (u32) strlen(processors[i].szName));
		if (strcmp(instanceName.data, "_Total") == 0)
		{
			AddSensor(context, api, InstanceType::Load,  instanceName, "CPU Load",  "windows/cpu/load",  "%.0f%%");
			AddSensor(context, api, InstanceType::Clock, instanceName, "CPU Clock", "windows/cpu/clock", "%.0f MHz");
			continue;
		}

		u32 node  = 0;
		u32 index = 0;
		i32 end   = 0;
		c8 labelBuf[32];
		StringView label = instanceName;
		if (sscanf_s(instanceName.data, "%u,_Total%n", &node, &end) == 1 && (u32) end == instanceName.length)
		{
			label = String_FormatInto(labelBuf, "Node %", node);
		}
		else if (sscanf_s(instanceName.data, "%u,%u", &node, &index) == 2)
		{
			label = highestNode == 0
				? String_FormatInto(labelBuf, "Thread %", index)
				: String_FormatInto(labelBuf, "Node % Thread %", node, index);
		}

		name       = String_FormatInto(nameBuf,       "CPU % Load",         label);
		identifier = String_FormatInto(identifierBuf, "windows/cpu/%/load", instanceName);
		AddSensor(context, api, InstanceType::Load, instanceName, name, identifier, "%.0f%%");

		name       = String_FormatInto(nameBuf,       "CPU % Clock",         label);
		identifier = String_FormatInto(identifierBuf, "windows/cpu/%/clock", instanceName);
		AddSensor(context, api, InstanceType::Clock, instanceName, name, identifier, "%.0f MHz");
	}

	// _Total                       - Sum of the below
	// RAPL_Package<N>_PKG          - All physical processors in socket
	// RAPL_Package<N>_Core<N>_CORE - One physical processor in socket
	// RAPL_Package<N>_PP<N>        -
	// RAPL_Package<N>_DRAM         -
	if (state.power)
	{
		Slice<PDH_RAW_COUNTER_ITEM_A> meters = GetInstances(state.power);
		for (u32 i = 0; i < meters.length; i++)
		{
			StringView instanceName(meters[i].szName, (u32) strlen(meters[i].szName));

			u32 package = 0;
			u32 core    = 0;
			i32 end     = 0;
			if (sscanf_s(instanceName.data, "RAPL_Package%u_PKG%n", &package, &end) == 1 && (u32) end == instanceName.length)
			{
				name       = String_FormatInto(nameBuf,       "CPU Package % Power",         package);
				identifier = String_FormatInto(identifierBuf, "windows/cpu/package/%/power", package);
			}
			else if (sscanf_s(instanceName.data, "RAPL_Package%u_Core%u_CORE%n", &package, &core, &end) == 2 && (u32) end == instanceName.length)
			{
				name       = String_FormatInto(nameBuf,       "CPU Core % Power", core);
				identifier = String_FormatInto(identifierBuf, "windows/cpu/package/%/core/%/power", package, core);
			}
			else
			{
				name       = String_FormatInto(nameBuf,       "Energy Meter %",   instanceName);
				identifier = String_FormatInto(identifierBuf, "windows/energy/%", instanceName);
			}
			AddSensor(context, api, InstanceType::Power, instanceName, name, identifier, "%.1f W");
		}
	}

	queryGuard.dismiss = true;
	return true;
}

static void
Update(PluginContext& context, SensorPluginAPI::Update api)
{
	Unused(context);

	u64 nowMs = GetTickCount64();
	if (nowMs - state.lastSampleMs < state.sampleIntervalMs) return;
	state.lastSampleMs = nowMs;

	PDH_STATUS status = PdhCollectQueryData(state.query);
	if (status != ERROR_SUCCESS) return;

	Slice<PDH_FMT_COUNTERVALUE_ITEM_A> utility = GetValues(state.utility);
	for (u32 i = 0; i < utility.length; i++)
	{
		PDH_FMT_COUNTERVALUE_ITEM_A& item = utility[i];
		Instance* instance = FindInstance(InstanceType::Load, item.szName);
		if (!instance || !IsValid(item)) continue;

		u32 index = List_PointerToIndex(state.instances, *instance);
		api.sensors[index].value = (r32) item.FmtValue.doubleValue;
	}

	Slice<PDH_FMT_COUNTERVALUE_ITEM_A> frequency = GetValues(state.frequency);
	for (u32 i = 0; i < frequency.length; i++)
	{
		PDH_FMT_COUNTERVALUE_ITEM_A& item = frequency[i];
		Instance* instance = FindInstance(InstanceType::Clock, item.szName);
		if (!instance || !IsValid(item)) continue;

		u32 index = List_PointerToIndex(state.instances, *instance);
		api.sensors[index].value = (r32) item.FmtValue.doubleValue;
	}

	// Energy Meter reports milliwatts
	if (state.power)
	{
		Slice<PDH_FMT_COUNTERVALUE_ITEM_A> power = GetValues(state.power);
		for (u32 i = 0; i < power.length; i++)
		{
			PDH_FMT_COUNTERVALUE_ITEM_A& item = power[i];
			Instance* instance = FindInstance(InstanceType::Power, item.szName);
			if (!instance || !IsValid(item)) continue;

			u32 index = List_PointerToIndex(state.instances, *instance);
			api.sensors[index].value = (r32) (item.FmtValue.doubleValue / 1000.0);
		}
	}
}

static void
Teardown(PluginContext& context, SensorPluginAPI::Teardown api)
{
	Unused(context, api);

	PdhCloseQuery(state.query);

	for (u32 i = 0; i < state.instances.length; i++)
		String_Free(state.instances[i].name);
	List_Free(state.instances);
	List_Free(state.buffer);

	state = {};
}

EXPORT void
GetSensorPluginInfo(PluginDesc& desc, SensorPluginFunctions& functions)
{
	desc.name       = "Windows Sensors";
	desc.author     = "akbyrd";
	desc.version    = 1;
	desc.lhmVersion = LHMVersion;

	functions.Initialize = Initialize;
	functions.Update     = Update;
	functions.Teardown   = Teardown;
}
